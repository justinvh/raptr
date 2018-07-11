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
#include <raptr/config.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/sound/sound.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/common/rtree.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/game/entity.hpp>
#include <raptr/network/snapshot.hpp>

namespace { auto logger = raptr::_get_logger(__FILE__); };

namespace raptr {

Character::Character()
  : Entity()
{
  fast_fall_scale = 1.0;
  jump_perfect_scale = 1.0;
  walk_speed = 100;
  run_speed = 100;
  jump_vel = 100;
  fast_fall = false;
  falling = false;
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
    "character.walk_speed",
    "character.run_speed",
    "character.jump_vel",
    "character.jumps_allowed",
    "character.jump_perfect_scale",
    "character.fast_fall_scale",
    "character.dash_speed",
    "character.dash_length_ms",
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
  character->walk_speed = dict["character.walk_speed"]->as<int32_t>();
  character->run_speed = dict["character.run_speed"]->as<int32_t>();
  character->jump_vel = dict["character.jump_vel"]->as<int32_t>();
  character->jumps_allowed = dict["character.jumps_allowed"]->as<int32_t>();
  character->jump_perfect_scale = dict["character.jump_perfect_scale"]->as<double>();
  character->fast_fall_scale = dict["character.fast_fall_scale"]->as<double>();
  character->sprite->x = 0;
  character->sprite->y = 0;
  character->jump_count = 0;
  character->dash_speed = dict["character.dash_speed"]->as<int32_t>();
  character->dash_length_us = dict["character.dash_length_ms"]->as<int32_t>() * 1e3;
  character->dash_time_us = 0;

  character->do_pixel_collision_test = false;
  if (character->sprite->has_animation("Collision")) {
    character->collision_frame = &character->sprite->animations["Collision"].frames[0];
    character->do_pixel_collision_test = true;
  }

  auto& pos = character->position();
  auto& vel = character->velocity();
  auto& acc = character->acceleration();
  pos.x = 0; pos.y = 0;
  vel.x = 0; vel.y = 0;
  acc.x = 0; acc.y = 0;

  return character;
}

void Character::crouch()
{
  sprite->set_animation("Crouch", true);
}

void Character::think(std::shared_ptr<Game>& game)
{
  int64_t delta_us = game->frame_delta_us;
  auto& vel = this->velocity();
  auto& acc = this->acceleration();
  auto& pos = this->position();

  if (jump_count) {
    jump_time_us += delta_us;
  }

  bool in_dash = false;
  if (dash_time_us > 0) {
    in_dash = true;
    dash_time_us += delta_us;
  }

  if (dash_time_us > dash_length_us) {
    in_dash = false;
    dash_time_us = 0;
    this->on_left_joy(this->last_controller_state);
  }

  acc.y = game->gravity;

  // External forces, like gravity
  Rect fall_check = this->want_position_y(delta_us)[0];
  fall_check.y += 0.1;

  auto intersected_entity = game->intersect_world(this, fall_check);
  if (!intersected_entity && !in_dash) {
    if (fast_fall) {
      vel.y += fast_fall_scale * game->gravity * delta_us / 1e6;
    } else {
      vel.y += game->gravity * delta_us / 1e6;
    }
    falling = true;
  } else if (!in_dash) {
    if (falling) {
      jump_count = 0;
      jump_time_us = 0;
      fast_fall = false;
      vel.y = 0;

      if (!moving) {
        vel.x = 0;
      }

      if (in_dash) {
        sprite->set_animation("Dash");
      } else if (vel.x > walk_speed) {
        sprite->set_animation("Run");
      } else if (vel.x > 0) {
        sprite->set_animation("Walk");
      } else {
        sprite->set_animation("Idle");
      }
    }
    falling = false;
  }

  Rect want_x = this->want_position_x(delta_us)[0];
  Rect want_y = this->want_position_y(delta_us)[0];

  int32_t steps_x = static_cast<int32_t>(std::abs(pos.x - want_x.x) / 4 + 1);
  double delta_x = (pos.x - want_x.x) / double(steps_x);
  bool intersected = false;
  want_x.x = pos.x;
  for (int32_t i = 1; i <= steps_x; ++i) {
    want_x.x -= delta_x * i;
    intersected |= game->intersect_world(this, want_x) != nullptr;
    if (intersected) {
      break;
    }
  }

  if (!intersected) {
    if (vel.x < 0) {
      sprite->flip_x = false;
    } else if (vel.x > 0) {
      sprite->flip_x = true;
    }

    // Is there something above us?
    Rect above_check = this->want_position_y(delta_us)[0];
    above_check.y -= 1;
    auto& intersected = game->intersect_world(this, above_check);
    Character* character = dynamic_cast<Character*>(intersected.get());
    if (character && !character->moving) {
      auto& ov = intersected->velocity();
      ov.x = vel.x;
    }
    pos.x = want_x.x;
  } else {
    vel.x = 0;
    dash_time_us = 0;
    sprite->set_animation("Idle");
  }

  int32_t steps_y = static_cast<int32_t>((std::abs(pos.y - want_y.y) / 4) + 1);
  double delta_y = (pos.y - want_y.y) / double(steps_y);
  want_y.y = pos.y;
  intersected_entity.reset();

  for (int32_t i = 1; i <= steps_y; ++i) {
    want_y.y -= delta_y * i;
    intersected_entity = game->intersect_world(this, want_y);
    if (intersected_entity) {
      break;
    }
  }

  if (!intersected_entity) {
    pos.y = want_y.y;
  } else {
    Character* character = dynamic_cast<Character*>(intersected_entity.get());
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

  sprite->x = pos.x;
  sprite->y = pos.y;
}

void Character::walk(float scale)
{
  moving = true;
  auto& vel = this->velocity();
  vel.x = scale * run_speed;
  if (!falling) {
    sprite->set_animation("Walk");
  }
}

void Character::run(float scale)
{
  moving = true;
  auto& vel = this->velocity();
  vel.x = scale * run_speed;

  if (!falling) {
    sprite->set_animation("Run");
  }
}

void Character::stop()
{
  moving = false;
  auto& vel = this->velocity();
  vel.x = 0.0;
  dash_time_us = 0;
  if (!falling) {
    sprite->set_animation("Idle");
  }
}

bool Character::on_left_joy(const ControllerState& state)
{
  this->last_controller_state = state;
  auto& vel = this->velocity();
  float mag_x = std::fabs(state.x);

  if (dash_time_us > 0) {
    if (mag_x < 0.01f && !falling) {
      this->stop();
    
    }
    return false;
  }

  // Turn around boosted dash
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

  return false;
}

bool Character::on_button_down(const ControllerState& state)
{
  auto& vel = this->velocity();
  auto& acc = this->acceleration();
  if (state.button == Button::a && jump_count < jumps_allowed) {
    int64_t peak_time_us = static_cast<int64_t>(std::fabs(jump_vel / acc.y * 1e6));
    int64_t in_air_us = clock::ticks() - jump_time_us;

    if (peak_time_us - in_air_us < 64 * 1e6 && falling) {
      vel.y -= -jump_vel * jump_perfect_scale;
    } else {
      vel.y -= -jump_vel;
    }

    jump_time_us = clock::ticks();
    sprite->set_animation("Jump");
    ++jump_count;
  }

  if (state.button == Button::x && dash_time_us == 0 && jump_count < jumps_allowed) {
    sprite->set_animation("Dash");
    dash_time_us = 1;
    if (sprite->flip_x) {
      vel.x += dash_speed;
    } else {
      vel.x -= dash_speed;
    } 


    vel.y = 0;
  }
  return false;
}

void Character::render(Renderer* renderer)
{
  sprite->render(renderer);

  if (flashlight) {
    auto& s1 = sprite->current_animation->current_frame();
    auto& s2 = flashlight_sprite->current_animation->current_frame();
    double cx = sprite->x + s1.w / 2.0 - s2.w / 2.0;
    double cy = sprite->y + s1.h / 2.0 - s2.h / 2.0;
    flashlight_sprite->x = cx;
    flashlight_sprite->y = cy;
    flashlight_sprite->render(renderer);
  }
}

void Character::attach_controller(std::shared_ptr<Controller>& controller_)
{
  using std::placeholders::_1;

  controller = controller_;
  controller->on_left_joy(std::bind(&Character::on_left_joy, this, _1));
  controller->on_button_down(std::bind(&Character::on_button_down, this, _1));
}

std::vector<Rect> Character::bbox() const
{
  Rect box;
  auto& pos = this->position();
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = pos.x;
  box.y = pos.y;
  box.w = (current_frame.w * sprite->scale);
  box.h = (current_frame.h * sprite->scale);
  return {box};
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
    CNF(walk_speed),
    CNF(run_speed),
    CNF(jump_vel),
    CNF(bunny_hop_count)
  };

  #undef NF

  for (auto state : states) {
    list.push_back(state);
  }
}

bool Character::deserialize(const std::vector<NetField>& fields)
{
  return false;
}

} // namespace raptr