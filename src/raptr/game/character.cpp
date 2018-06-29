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

macro_enable_logger();

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
  int64_t delta_ms = game->frame_delta_ms;
  auto& vel = this->velocity();
  auto& acc = this->acceleration();
  auto& pos = this->position();

  if (jump_count) {
    jump_time_ms += delta_ms;
  }

  acc.y = game->gravity;

  // External forces, like gravity
  Rect fall_check = this->want_position_y(delta_ms);
  fall_check.y += 0.1;

  auto intersected_entity = game->intersect_world(this, fall_check);
  if (!intersected_entity) {
    if (fast_fall) {
      vel.y += fast_fall_scale * game->gravity * delta_ms / 1000.0;
    } else {
      vel.y += game->gravity * delta_ms / 1000.0;
    }
    falling = true;
  } else {
    if (falling) {
      jump_count = 0;
      jump_time_ms = 0;
      fast_fall = false;
      vel.y = 0;
    }
    falling = false;
  }

  Rect want_x = this->want_position_x(delta_ms);
  Rect want_y = this->want_position_y(delta_ms);

  if (!game->intersect_world(this, want_x)) {
    if (vel.x < 0) {
      sprite->flip_x = false;
    } else if (vel.x > 0) {
      sprite->flip_x = true;
    }
    pos.x = want_x.x;
  }

  if (!game->intersect_world(this, want_y)) {
    pos.y = want_y.y;
  } 

  sprite->x = pos.x;
  sprite->y = pos.y;

  sprite->render(game->renderer);
}

void Character::walk(float scale)
{
  auto& vel = this->velocity();
  vel.x = scale * walk_speed;
  if (!falling) {
    sprite->set_animation("Walk");
  }
}

void Character::run(float scale)
{
  auto& vel = this->velocity();
  vel.x = scale * run_speed;

  if (!falling) {
    sprite->set_animation("Run");
  }
}

void Character::stop()
{
  auto& vel = this->velocity();
  vel.x = 0.0;
  sprite->set_animation("Idle");
}

bool Character::on_right_joy(const ControllerState& state)
{
  float mag_x = std::fabs(state.x);

  if (mag_x < 0.01f) {
    this->stop();
  } else if (mag_x < 0.5f) {
    this->walk(state.x / 0.5f);
  } else if (mag_x >= 0.5f) {
    this->run(state.x / 1.0f);
  }

  if (falling && state.y > 0.5) {
    fast_fall = true;
  }

  return true;
}


bool Character::on_button_down(const ControllerState& state)
{
  auto& vel = this->velocity();
  auto& acc = this->acceleration();
  if (state.button == Button::a && jump_count < jumps_allowed) {
    int32_t peak_time_ms = static_cast<int32_t>(std::fabs(jump_vel / acc.y * 1000.0));
    int32_t button_time = std::abs(jump_time_ms - peak_time_ms);

    if (falling && button_time < 32) {
      vel.y -= -jump_vel * jump_perfect_scale;
    } else {
      vel.y -= -jump_vel * 1.0 / (jump_count + 1.0);
    }

    jump_time_ms = 0;
    logger->debug("Jump should take {}ms. Clicked at {}. Delta is {}", peak_time_ms, jump_time_ms, button_time);
    sprite->set_animation("Jump");
    ++jump_count;
  }
  return true;
}

void Character::attach_controller(std::shared_ptr<Controller>& controller_)
{
  using std::placeholders::_1;

  controller = controller_;
  controller->on_right_joy(std::bind(&Character::on_right_joy, this, _1));
  controller->on_button_down(std::bind(&Character::on_button_down, this, _1));
}

bool Character::intersects(const Entity* other) const
{
  if (other->id() == this->id()) {
    return false;
  }

  Rect self_box = this->bbox();
  Rect other_box = other->bbox();
  Rect res_box;
  if (!SDL_IntersectRect(&self_box, &other_box, &res_box)) {
    return false;
  }

  std::clog << other->id() << " is intersecting with " << this->id() << "\n";

  return true;
}

Rect Character::bbox() const
{
  Rect box;
  auto& pos = this->position();
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = pos.x;
  box.y = pos.y;
  box.w = (current_frame.w * sprite->scale);
  box.h = (current_frame.h * sprite->scale);
  return box;
}

} // namespace raptr
