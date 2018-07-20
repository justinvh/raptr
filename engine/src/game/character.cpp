#include <functional>
#include <memory>
#include <string>
#include <map>

#pragma warning(disable : 4996)
#include <toml/toml.h>
#pragma warning(default : 4996)

#include <raptr/game/game.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/game/character.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/sound/sound.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/common/rtree.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/game/entity.hpp>
#include <raptr/network/snapshot.hpp>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr
{

Character::Character()
  : Entity()
{
  fast_fall_scale = 1.0;
  jump_perfect_scale = 1.0;
  walk_speed_ps = 100;
  run_speed_ps = 100;
  jump_vel_ps = 100;
  fast_fall = false;
  falling = false;
}

void Character::attach_controller(std::shared_ptr<Controller>& controller_)
{
  using std::placeholders::_1;

  controller = controller_;
  controller->on_left_joy(std::bind(&Character::on_left_joy, this, _1));
  controller->on_button_down(std::bind(&Character::on_button_down, this, _1));
  controller->on_button_up(std::bind(&Character::on_button_up, this, _1));
}

std::vector<Rect> Character::bbox() const
{
  Rect box;
  auto& pos = this->position();
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = pos.x;
  box.y = pos.y;
  box.w = current_frame.w * sprite->scale;
  box.h = current_frame.h * sprite->scale;
  return {box};
}

void Character::crouch()
{
  sprite->set_animation("Crouch", true);
}

bool Character::deserialize(const std::vector<NetField>& fields)
{
  return false;
}

std::shared_ptr<Character> Character::from_toml(const FileInfo& toml_path)
{
  auto toml_relative = toml_path.file_relative;
  auto ifs = toml_path.open();
  if (!ifs) {
    return nullptr;
  }

  toml::ParseResult pr = toml::parse(*ifs);

  if (!pr.valid()) {
    logger->error("Failed to parse {} with reason {}", toml_relative, pr.errorReason);
    return nullptr;
  }

  const toml::Value& v = pr.value;

  std::string toml_keys[] = {
    "character.name",
    "character.walk_speed_kmh",
    "character.run_speed_kmh",
    "character.jump_vel_ms",
    "character.mass_kg",
    "character.jumps_allowed",
    "character.jump_perfect_scale",
    "character.fast_fall_scale",
    "character.dash_speed_kmh",
    "character.dash_length_msec",
    "sprite.path",
    "sprite.scale"
  };

  std::map<std::string, const toml::Value*> dict;

  for (const auto& key : toml_keys) {
    const toml::Value* value = v.find(key);
    if (!value) {
      logger->error("{} is missing {}", toml_relative, key);
      return nullptr;
    }
    dict[key] = value;
  }

  std::string sprite_path = dict["sprite.path"]->as<std::string>();
  if (!fs::exists(toml_path.game_root / sprite_path)) {
    logger->error("{} is not a valid sprite path in {}", sprite_path, toml_relative);
  }

  std::shared_ptr<Character> character(new Character());
  FileInfo sprite_file;
  sprite_file.game_root = toml_path.game_root;
  sprite_file.file_path = toml_path.game_root / sprite_path;
  sprite_file.file_relative = sprite_path;
  sprite_file.file_dir = sprite_file.file_path.parent_path();

  character->flashlight_sprite = Sprite::from_json(toml_path.from_root("textures/flashlight.json"));
  character->flashlight_sprite->blend_mode = SDL_BLENDMODE_ADD;
  character->flashlight_sprite->render_in_foreground = true;
  character->sprite = Sprite::from_json(sprite_file);
  character->sprite->scale = dict["sprite.scale"]->as<double>();
  character->sprite->set_animation("Idle");
  character->walk_speed_ps = dict["character.walk_speed_kmh"]->as<int32_t>() * kmh_to_ps;
  character->run_speed_ps = dict["character.run_speed_kmh"]->as<int32_t>() * kmh_to_ps;
  character->jump_vel_ps = dict["character.jump_vel_ms"]->as<int32_t>() * ms_to_ps;
  character->mass_kg = dict["character.mass_kg"]->as<double>();

  character->jumps_allowed = dict["character.jumps_allowed"]->as<int32_t>();
  character->jump_perfect_scale = dict["character.jump_perfect_scale"]->as<double>();
  character->fast_fall_scale = dict["character.fast_fall_scale"]->as<double>();
  character->sprite->x = 0;
  character->sprite->y = 0;
  character->jump_count = 0;
  character->dash_speed_ps = dict["character.dash_speed_kmh"]->as<int32_t>() * kmh_to_ps;
  character->dash_length_usec = static_cast<int32_t>(dict["character.dash_length_msec"]->as<int32_t>() * 1e3);
  character->dash_time_usec = 0;

  character->do_pixel_collision_test = false;
  if (character->sprite->has_animation("Collision")) {
    character->collision_frame = &character->sprite->animations["Collision"].frames[0];
    character->do_pixel_collision_test = true;
  }

  auto& pos = character->position();
  auto& vel = character->velocity();
  auto& acc = character->acceleration();
  pos.x = 0;
  pos.y = 0;
  vel.x = 0;
  vel.y = 0;
  acc.x = 0;
  acc.y = 0;
  character->vel_exp.x = 0;

  return character;
}

bool Character::on_button_down(const ControllerState& state)
{
  auto& vel = this->velocity();
  auto& acc = this->acceleration();
  if (state.button == Button::a && jump_count < jumps_allowed) {
    if (gravity_ps2 < 0) {
      vel.y = jump_vel_ps;
    } else {
      vel.y = -jump_vel_ps;
    }

    jump_time_us = clock::ticks();
    sprite->set_animation("Jump");
    ++jump_count;
    dash_time_usec = 0;
  } else if (state.button == Button::y) {
    sprite->flip_x = !sprite->flip_x;
  } else if (state.button == Button::x && dash_time_usec == 0 && jump_count <= jumps_allowed) {
    sprite->set_animation("Dash");
    dash_time_usec = 1;
    if (sprite->flip_x) {
      vel.x += dash_speed_ps;
    } else {
      vel.x -= dash_speed_ps;
    }

    vel.y = 0;
    jump_count++;
  }
  return false;
}

bool Character::on_button_up(const ControllerState& state)
{
  auto& vel = this->velocity();
  if (state.button == Button::a) {
    if (vel.y > 0) {
      vel.y = 0;
    }
  }
  return true;
}

bool Character::on_left_joy(const ControllerState& state)
{
  auto& vel = this->velocity();
  float mag_x = std::fabs(state.x);

  if (mag_x < 0.01f) {
    this->stop();
  } else if (mag_x < 0.75f) {
    this->walk(state.x);
  } else if (mag_x >= 0.75f) {
    this->run(state.x);
  }

  if (falling && state.y > 0.5) {
    fast_fall = true;
  }

  this->last_controller_state = state;
  return false;
}

void Character::render(Renderer* renderer)
{
  sprite->render(renderer);

  if (flashlight) {
    const auto& s1 = sprite->current_animation->current_frame();
    const auto& s2 = flashlight_sprite->current_animation->current_frame();
    const double cx = sprite->x + s1.w / 2.0 - s2.w / 2.0;
    const double cy = renderer->window_size.h - sprite->y - s1.h / 2.0 - s2.h / 2.0;
    flashlight_sprite->x = cx;
    flashlight_sprite->y = cy;
    flashlight_sprite->render(renderer);
  }
}

void Character::run(float scale)
{
  moving = true;
  auto& vel = this->velocity();

  if (vel.x > 0 && scale < 0 || vel.x < 0 && scale > 0) {
    dash_time_usec = 0;
    vel.x = scale * run_speed_ps;
  }

  if (std::fabs(scale * run_speed_ps) > std::fabs(vel.x)) {
    vel.x = scale * run_speed_ps;
  }
  vel_exp.x = scale * run_speed_ps;

  //logger->debug("Run speed is {}m/s.", vel.x * pixels_to_meters);

  if (!falling) {
    sprite->set_animation("Run");
    sprite->speed = std::fabs(scale * 2.0);
  }
}

void Character::serialize(std::vector<NetField>& list)
{
  NetFieldType cls = NetFieldType::Character;

  // Stop looking at me like that.
  #define CNF(field) NetFieldMacro(Character, field)

  NetField states[] = {
    CNF(pos_.x),
    CNF(pos_.y),
    CNF(vel_.x),
    CNF(vel_.y),
    CNF(moving),
    CNF(flashlight),
    CNF(falling),
    CNF(fast_fall),
    CNF(fast_fall_scale),
    CNF(jump_time_us),
    CNF(jump_count),
    CNF(jumps_allowed),
    CNF(jump_perfect_scale),
    CNF(walk_speed_ps),
    CNF(run_speed_ps),
    CNF(jump_vel_ps),
    CNF(bunny_hop_count)
  };

  #undef NF

  for (auto state : states) {
    list.push_back(state);
  }
}

void Character::stop()
{
  moving = false;
  auto& vel = this->velocity();
  dash_time_usec = 0;
  vel_exp.x = 0;
  sprite->speed = 1.0;
  if (!falling) {
    sprite->set_animation("Idle");
  }

  if (falling) {
    vel.x = 0;
  }
}

void Character::think(std::shared_ptr<Game>& game)
{
  const auto delta_us = game->frame_delta_us;
  const auto delta_s = delta_us / 1e6;
  auto& vel = this->velocity();
  auto& acc = this->acceleration();
  auto& pos = this->position();

  if (jump_count) {
    jump_time_us += delta_us;
  }

  bool in_dash = false;
  if (dash_time_usec > 0) {
    in_dash = true;
    dash_time_usec += delta_us;
  }

  if (dash_time_usec > dash_length_usec) {
    in_dash = false;
    dash_time_usec = 0;
    this->on_left_joy(this->last_controller_state);
  }

  acc.y = gravity_ps2;
  if (in_dash) {
    acc.y = 0;
  }

  // External forces, like gravity
  Rect fall_check = this->want_position_y(delta_us)[0];
  if (gravity_ps2 < 0) {
    fall_check.y -= 0.05;
    this->sprite->flip_y = false;
  } else {
    fall_check.y += 0.05;
    this->sprite->flip_y = true;
  }

  auto intersected_entity = game->intersect_entity(this, fall_check);
  if (!intersected_entity && !in_dash) {
    if (fast_fall) {
      vel.y += fast_fall_scale * gravity_ps2 * delta_us / 1e6;
    } else {
      vel.y += gravity_ps2 * delta_us / 1e6;
    }
    fall_time_us += delta_us;
    falling = true;
  } else if (!in_dash) {
    if (falling) {
      jump_count = 0;
      jump_time_us = 0;
      fast_fall = false;
      //logger->debug("Fell for {}s. Final velocity was {}m/s", fall_time_us / 1e6, vel.y * pixels_to_meters);
      vel.y = 0;

      float mag_x = std::fabs(vel.x);
      if (in_dash) {
        sprite->set_animation("Dash");
      } else if (mag_x > walk_speed_ps) {
        sprite->set_animation("Run");
      } else if (mag_x > 0) {
        sprite->set_animation("Walk");
      } else {
        sprite->set_animation("Idle");
      }
    }
    falling = false;
    fall_time_us = 0;
  }

  float friction = 3000;

  if (in_dash) {
    friction /= 2;
  }

  if (falling) {
    friction /= 2;
  }

  if (std::fabs(vel_exp.x) < std::fabs(vel.x)) {
    if (vel.x > 0) {
      vel.x -= friction * delta_us / 1e6;
      if (vel.x < 0) {
        vel.x = 0;
      }
    } else if (vel.x < 0) {
      vel.x += friction * delta_us / 1e6;
      if (vel.x > 0) {
        vel.x = 0;
      }
    }
  }

  const auto mag_x = std::fabs(vel.x);

  Rect want_x = this->want_position_x(delta_us)[0];
  Rect want_y = this->want_position_y(delta_us)[0];

  const auto steps_x = static_cast<int32_t>(std::abs(pos.x - want_x.x) / 4 + 1);
  const auto delta_x = (pos.x - want_x.x) / double(steps_x);
  auto intersected = false;
  want_x.x = pos.x;
  for (auto i = 1; i <= steps_x; ++i) {
    want_x.x -= delta_x * i;
    intersected |= game->intersect_entity(this, want_x) != nullptr;
    if (intersected) {

      break;
    }
  }

  bool hitting_wall = false;
  
  if (!intersected) {
    if (vel.x < 0) {
      sprite->flip_x = false;
    } else if (vel.x > 0) {
      sprite->flip_x = true;
    }

    // Is there something above us?
    Rect above_check = this->want_position_y(delta_us)[0];
    above_check.y -= 1;
    auto intersected = game->intersect_entity(this, above_check);
    Character* character = dynamic_cast<Character*>(intersected.get());
    if (character && !character->moving) {
      auto& ov = intersected->velocity();
      ov.x = vel.x;
    }
    pos.x = want_x.x;
  } else {
    hitting_wall = true;
    dash_time_usec = 0;
    sprite->set_animation("Idle");
  }

  const auto steps_y = static_cast<int32_t>(std::abs(pos.y - want_y.y) / 4 + 1);
  const auto delta_y = (pos.y - want_y.y) / double(steps_y);
  want_y.y = pos.y;
  intersected_entity.reset();

  for (int32_t i = 1; i <= steps_y; ++i) {
    want_y.y -= delta_y * i;
    intersected_entity = game->intersect_entity(this, want_y);
    if (intersected_entity) {
      break;
    }
  }

  if (!intersected_entity) {
    pos.y = want_y.y;
  } else {
    const auto character = dynamic_cast<Character*>(intersected_entity.get());
    if (character) {
      auto& ov = intersected_entity->velocity();
      ov.y = vel.y;
    } else {
      vel.y = 0;
    }

    if (!falling) {
      jump_count = 0;
    }
  }

  if (in_dash) {
    sprite->set_animation("Dash");
  } else if (hitting_wall) {
    sprite->set_animation("Idle");
  } else if (falling) {
    sprite->set_animation("Jump");
  } else if (mag_x > walk_speed_ps) {
    sprite->set_animation("Run");
  } else if (mag_x > 0) {
    sprite->set_animation("Walk");
  } else {
    sprite->set_animation("Idle");
  }

  sprite->x = pos.x;
  sprite->y = pos.y;
}

void Character::walk(float scale)
{
  moving = true;
  auto& vel = this->velocity();
  if (std::fabs(scale * run_speed_ps) > std::fabs(vel.x)) {
    vel.x = scale * run_speed_ps;
  }
  vel_exp.x = scale * run_speed_ps;

  if (!falling) {
    sprite->set_animation("Walk");
    sprite->speed = std::fabs(scale * 2.0);
  }
}

} // namespace raptr
