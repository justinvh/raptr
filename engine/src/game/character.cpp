#include <functional>
#include <memory>
#include <string>
#include <map>
#include <thread>

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
  think_frame = 0;
  fast_fall_scale = 1.0;
  jump_perfect_scale = 1.0;
  walk_speed_ps = 100;
  run_speed_ps = 100;
  jump_vel_ps = 100;
  fast_fall = false;
  is_falling = false;
  is_tweening = false;
}

void Character::attach_controller(std::shared_ptr<Controller>& controller_)
{
  using std::placeholders::_1;

  if (controller) {
    this->detach_controller();
  }

  controller = controller_;
  on_left_joy_id = controller->on_left_joy(std::bind(&Character::on_left_joy, this, _1));
  on_button_down_id = controller->on_button_down(std::bind(&Character::on_button_down, this, _1));
  on_button_up_id = controller->on_button_up(std::bind(&Character::on_button_up, this, _1));
}

void Character::detach_controller()
{
  if (!controller) {
    return;
  }
  controller->unbind({on_left_joy_id, on_button_down_id, on_button_up_id});
}


std::vector<Rect> Character::bbox() const
{
  Rect box;
  auto pos = this->position_abs();
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = pos.x;
  box.y = pos.y;
  box.w = current_frame.w * sprite->scale;
  box.h = current_frame.h * sprite->scale;
  return {box};
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
    "sprite.scale",
    "script.path",
  };

  std::map<std::string, const toml::Value*> dict;

  for (const auto& key : toml_keys) {
    const toml::Value* value = v.find(key);
    if (!value) {
      logger->warn("{} is missing {}", toml_relative, key);
      continue;
    }
    dict[key] = value;
  }

  const auto sprite_path = dict["sprite.path"]->as<std::string>();
  auto full_sprite_path = toml_path.file_dir / sprite_path;
  if (!fs::exists(full_sprite_path)) {
    full_sprite_path = toml_path.game_root / sprite_path;
    if (!fs::exists(full_sprite_path)) {
      logger->error("{} is not a valid sprite path in {}", sprite_path, toml_relative);
      return nullptr;
    }
  }

  auto V = [&](const std::string& key, auto default_value) -> decltype(default_value)
  {
    const auto found = dict.find(key);
    if (found == dict.end()) {
      logger->warn("Defaulting {} to {}", key, default_value);
      return default_value;
    }

    return found->second->as<decltype(default_value)>();
  };

  std::shared_ptr<Character> character(new Character());

  FileInfo sprite_file;
  sprite_file.game_root = toml_path.game_root;
  sprite_file.file_path = full_sprite_path;
  sprite_file.file_relative = sprite_path;
  sprite_file.file_dir = sprite_file.file_path.parent_path();

  character->flashlight_sprite = Sprite::from_json(toml_path.from_root("textures/flashlight.json"));
  character->flashlight_sprite->blend_mode = SDL_BLENDMODE_ADD;
  character->flashlight_sprite->render_in_foreground = true;
  character->sprite = Sprite::from_json(sprite_file);
  character->sprite->scale = V("sprite.scale", 1.0);
  character->set_animation("Idle");
  character->walk_speed_ps = V("character.walk_speed_kmh", 10.0) * kmh_to_ps;
  character->run_speed_ps = V("character.run_speed_kmh", 20.0) * kmh_to_ps;
  character->jump_vel_ps = V("character.jump_vel_ms", 25.0) * ms_to_ps;
  character->mass_kg = V("character.mass_kg", 100.0);

  character->jumps_allowed = V("character.jumps_allowed", 1);
  character->jump_perfect_scale = V("character.jump_perfect_scale", 1.25);
  character->fast_fall_scale = V("character.fast_fall_scale", 1.25);
  character->sprite->x = 0;
  character->sprite->y = 0;
  character->jump_count = 0;
  character->dash_speed_ps = V("character.dash_speed_kmh", 50.0) * kmh_to_ps;
  character->dash_length_usec = static_cast<int64_t>(V("character.dash_length_msec", 100) * 1e3);
  character->dash_time_usec = 0;

  character->do_pixel_collision_test = false;
  if (character->sprite->has_animation("Collision")) {
    character->collision_frame = &character->sprite->animations["Collision"].frames[0];
    character->do_pixel_collision_test = true;
  }

  character->is_scripted = false;
  const auto script_path_value = v.find("script.path");
  if (script_path_value) {
    const auto script_path = script_path_value->as<std::string>();
    auto full_script_path = toml_path.file_dir / script_path;
    if (!fs::exists(full_script_path)) {
      full_script_path = toml_path.game_root / script_path;
      if (!fs::exists(full_script_path)) {
        logger->error("{} is not a valid script path in {}", script_path, toml_relative);
        return nullptr;
      }
    }

    // The game will initialize this Lua object with other functions
    character->lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::io);

    FileInfo lua_script_fileinfo;
    lua_script_fileinfo.game_root = toml_path.game_root;
    lua_script_fileinfo.file_path = full_script_path;
    lua_script_fileinfo.file_relative = script_path;
    lua_script_fileinfo.file_dir = full_script_path.parent_path();

    auto script = lua_script_fileinfo.read();
    if (!script) {
      logger->error("{} failed to read!", lua_script_fileinfo);
      return nullptr;
    }

    character->lua_script_fileinfo = lua_script_fileinfo;
    character->lua_script = *script;
    character->is_scripted = true;
  }

  auto sounds_array_obj = v.find("sounds");
  if (sounds_array_obj) {
    size_t k = 0;
    auto sounds_array = sounds_array_obj->as<toml::Array>();
    for (auto& sound_group : sounds_array) {
      auto animation_obj = sound_group.find("animation");
      auto wav_obj = sound_group.find("wav");
      auto loop_obj = sound_group.find("loop");
      auto frame_obj = sound_group.find("frame");
      int32_t frame = 0;
      bool looping = false;

      if (!animation_obj) {
        logger->warn("Skipping [[sounds]] index at {} because it is missing animation key", k);
        continue;
      }

      const auto animation_name = animation_obj->as<std::string>();
      if (!character->sprite->has_animation(animation_name)) {
        logger->warn("Skipping [[sounds]] index at {} because {} is not an available animation",
          k, animation_name);
        continue;
      }

      if (!wav_obj) {
        logger->warn("Skipping [[sounds]] index at {} because it is missing wav key", k);
        continue;
      }

      const auto wav_path = wav_obj->as<std::string>();
      auto full_wav_path = toml_path.file_dir / wav_path;
      if (!fs::exists(full_wav_path)) {
        full_wav_path = toml_path.game_root / wav_path;
        if (!fs::exists(full_wav_path)) {
          logger->error("Skipping [[sounds]] index at {} because wav points to an invalid path {}",
            k, wav_path);
          continue;
        }
      }

      if (loop_obj) {
        looping = loop_obj->as<bool>();
      }

      if (frame_obj) {
        frame = frame_obj->as<int32_t>();
      }

      FileInfo wav_fileinfo;
      wav_fileinfo.game_root = toml_path.game_root;
      wav_fileinfo.file_path = full_wav_path;
      wav_fileinfo.file_relative = wav_path;
      wav_fileinfo.file_dir = full_wav_path.parent_path();

      character->sprite->register_sound_effect(animation_name, frame, wav_fileinfo, looping);
    }
  }

  auto& pos = character->position_rel();
  auto& vel = character->velocity_rel();
  auto& acc = character->acceleration_rel();
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
  if (state.button == Button::a) {
    this->jump();
  } else if (state.button == Button::y) {
    this->turn_around();
  } else if (state.button == Button::x) {
    this->dash();
  }

  return false;
}

void Character::walk_to(double x, double y)
{
  this->move_to(x, y, 0.5);
}

void Character::run_to(double x, double y)
{
  this->move_to(x, y, 1.0);
}

void Character::walk_to_rel(double x, double y)
{
  this->move_to_rel(x, y, 0.5);
}

void Character::run_to_rel(double x, double y)
{
  this->move_to_rel(x, y, 1.0);
}

void Character::move_to(double x, double y, float scale)
{
  is_tweening = true;

  auto walk_async = [&, x, y, scale]()
  {
    auto think_last_frame = think_frame;
    while (true) {
      if (think_last_frame == think_frame) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }
      think_last_frame = think_frame;
      const auto pos_abs = this->position_abs();
      const auto made_it = std::fabs(pos_abs.x - x) < 4; // within 4 pixels
      if (made_it) {
        this->stop();
        break;
      } else if (x > pos_abs.x) {
        this->walk(scale);
      } else {
        this->walk(-scale);
      }
    }
    is_tweening = false;
  };

  std::thread t1(walk_async);
  t1.detach();
}

void Character::move_to_rel(double x, double y, float scale)
{
  is_tweening = true;

  auto walk_async = [&, x, y, scale]()
  {
    auto dst = this->position_abs();
    dst.x += x;
    dst.y += y;

    auto think_last_frame = think_frame;
    while (true) {
      if (think_last_frame == think_frame) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }
      think_last_frame = think_frame;
      const auto pos_abs = this->position_abs();
      const auto made_it = std::fabs(pos_abs.x - dst.x) < 4; // within 4 pixels
      if (made_it) {
        this->position_rel().x += (pos_abs.x - dst.x);
        this->stop();
        break;
      } else if (dst.x > pos_abs.x) {
        this->walk(scale);
      } else {
        this->walk(-scale);
      }
    }

    is_tweening = false;
  };

  std::thread t1(walk_async);
  t1.detach();
}

void Character::jump()
{
  if (jump_count >= jumps_allowed) {
    return;
  }

  auto& vel = this->velocity_rel();
  if (gravity_ps2 < 0) {
    vel.y += jump_vel_ps;
  } else {
    vel.y -= jump_vel_ps;
  }
  jump_time_us = clock::ticks();
  this->set_animation("Jump");
  sprite->current_animation->sound_effect_has_played = false;
  dash_time_usec = 0;
  ++jump_count;
}

void Character::turn_around()
{
  sprite->flip_x = !sprite->flip_x;
}

void Character::dash()
{
  auto& vel = this->velocity_rel();
  if (dash_time_usec != 0) {
    return;
  }
  dash_time_usec = 1;
  if (sprite->flip_x) {
    vel.x += dash_speed_ps;
  } else {
    vel.x -= dash_speed_ps;
  }

  vel.y = 0;
  this->set_animation("Dash");
}

void Character::fall()
{
  auto& vel = this->velocity_rel();
  if (vel.y > 0) {
    vel.y = 0;
  }
}

bool Character::on_button_up(const ControllerState& state)
{
  if (is_dead) {
    return false;
  }

  if (state.button == Button::a) {
    this->fall();
  }
  return true;
}

bool Character::on_left_joy(const ControllerState& state)
{
  if (is_dead) {
    return false;
  }

  auto& vel = this->velocity_rel();
  float mag_x = std::fabs(state.x);

  if (mag_x < 0.01f) {
    this->stop();
  } else if (mag_x < 0.75f) {
    this->walk(state.x);
  } else if (mag_x >= 0.75f) {
    this->run(state.x);
  }

  if (is_falling && state.y > 0.5) {
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
    const double cy = sprite->y - s1.h / 2.0;
    flashlight_sprite->x = cx;
    flashlight_sprite->y = cy;
    flashlight_sprite->render(renderer);
  }
}

void Character::run(float scale)
{
  moving = true;
  auto& vel = this->velocity_rel();

  if (vel.x > 0 && scale < 0 || vel.x < 0 && scale > 0) {
    dash_time_usec = 0;
    vel.x = scale * run_speed_ps;
  }

  if (std::fabs(scale * run_speed_ps) > std::fabs(vel.x)) {
    vel.x = scale * run_speed_ps;
  }
  vel_exp.x = scale * run_speed_ps;

  //logger->debug("Run speed is {}m/s.", vel.x * pixels_to_meters);

  if (!is_falling) {
    this->set_animation("Run");
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
    CNF(is_falling),
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
  auto& vel = this->velocity_rel();
  dash_time_usec = 0;
  vel_exp.x = 0;
  sprite->speed = 1.0;
  if (!is_falling) {
    this->set_animation("Idle");
  }

  if (is_falling) {
    vel.x = 0;
  }
}

void Character::think(std::shared_ptr<Game>& game)
{
  const auto delta_us = game->frame_delta_us;
  const auto delta_s = delta_us / 1e6;
  auto& vel = this->velocity_rel();
  auto& acc = this->acceleration_rel();
  auto& pos = this->position_rel();

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
  if (gravity_ps2 <= 0) {
    fall_check.y -= 0.05;
    this->sprite->flip_y = false;
  } else {
    fall_check.y += 0.05;
    this->sprite->flip_y = true;
  }

  auto intersected = game->intersect_anything(this, fall_check);
  if (!intersected && !in_dash) {
    if (fast_fall) {
      vel.y += fast_fall_scale * gravity_ps2 * delta_us / 1e6;
    } else {
      vel.y += gravity_ps2 * delta_us / 1e6;
    }
    fall_time_us += delta_us;
    is_falling = true;
  } else if (!in_dash) {
    if (is_falling) {
      jump_count = 0;
      jump_time_us = 0;
      fast_fall = false;
      //logger->debug("Fell for {}s. Final velocity was {}m/s", fall_time_us / 1e6, vel.y * pixels_to_meters);
      vel.y = 0;

      const double mag_x = std::fabs(vel.x);
      if (in_dash) {
        this->set_animation("Dash");
      } else if (mag_x > walk_speed_ps) {
        this->set_animation("Run");
      } else if (mag_x > 0) {
        this->set_animation("Walk");
      } else {
        this->set_animation("Idle");
      }
    }
    is_falling = false;
    fall_time_us = 0;
  }

  float friction = 3000;

  if (in_dash) {
    friction /= 2;
  }

  if (is_falling) {
    friction /= 2;
  }

  if (is_dead && is_falling) {
    friction = 0;
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
  intersected = false;
  want_x.x = pos.x;
  for (auto i = 1; i <= steps_x; ++i) {
    want_x.x -= delta_x * i;
    intersected |= game->intersect_anything(this, want_x);
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
    above_check.y += 1;
    auto intersected = game->intersect_entity(this, above_check);
    Character* character = dynamic_cast<Character*>(intersected.get());
    if (character && !character->moving) {
      auto& ov = intersected->velocity_rel();
      ov.x = vel.x;
    }
    pos.x = want_x.x;
  } else {
    hitting_wall = true;
    dash_time_usec = 0;
    this->set_animation("Idle");
  }

  const auto steps_y = static_cast<int32_t>(std::abs(pos.y - want_y.y) / 4 + 1);
  const auto delta_y = (pos.y - want_y.y) / double(steps_y);
  want_y.y = pos.y;
  intersected = false;
  for (int32_t i = 1; i <= steps_y; ++i) {
    want_y.y -= delta_y * i;
    intersected = game->intersect_anything(this, want_y);
    if (intersected) {
      break;
    }
  }

  if (!intersected) {
    pos.y = want_y.y;
  } else {
    auto intersected_entity = game->intersect_entity(this, want_y);
    const auto character = dynamic_cast<Character*>(intersected_entity.get());
    if (character) {
      auto& ov = intersected_entity->velocity_rel();
      ov.y = vel.y;
    } else {
      vel.y = 0;
    }

    if (!is_falling) {
      jump_count = 0;
    }
  }

  if (in_dash) {
    this->set_animation("Dash");
  } else if (hitting_wall) {
    this->set_animation("Idle");
  } else if (is_falling) {
    this->set_animation("Jump");
  } else if (mag_x > walk_speed_ps) {
    this->set_animation("Run");
  } else if (mag_x > 0) {
    this->set_animation("Walk");
  } else {
    this->set_animation("Idle");
  }

  auto sprite_pos = this->position_abs();

  sprite->x = sprite_pos.x;
  sprite->y = sprite_pos.y;

  if (is_scripted) {
    lua["think"](delta_us);
  }

  ++think_frame;
}

void Character::walk(float scale)
{
  moving = true;
  auto& vel = this->velocity_rel();
  if (std::fabs(scale * run_speed_ps) > std::fabs(vel.x)) {
    vel.x = scale * run_speed_ps;
  }
  vel_exp.x = scale * run_speed_ps;

  if (!is_falling) {
    this->set_animation("Walk");
    sprite->speed = std::fabs(scale * 2.0);
  }
}

void Character::kill()
{
  is_dead = true;
  this->set_animation("Death");
  this->detach_controller();
}

void Character::set_animation(const std::string& name)
{
  if (is_dead && sprite->current_animation->name != "Death") {
    sprite->set_animation("Death", true);
    return;
  } else if (is_dead) {
    return;
  } else {
    sprite->set_animation(name);
  }
}

void Character::setup_lua_context(sol::state& state)
{
  state.new_usertype<Character>("Character",
    "add_velocity", &Character::add_velocity,
    "add_acceleration", &Character::add_acceleration,
    "attach_controller", &Character::attach_controller,
    "walk_to", &Character::walk_to,
    "walk_to_rel", &Character::walk_to_rel,
    "run_to", &Character::run_to,
    "run_to_rel", &Character::run_to_rel,
    "controller", &Character::controller,
    "is_tweening", &Character::is_tweening,
    "kill", &Character::kill,
    "jump", &Character::jump,
    "fall", &Character::fall,
    "dash", &Character::dash,
    "turn_around", &Character::turn_around,
    sol::base_classes, sol::bases<Entity>()
    );
}

} // namespace raptr
