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
  character->sprite->x = 0;
  character->sprite->y = 0;
  character->position().x = 0;
  character->position().y = 0;
  character->jump_vel = dict["character.jump_vel"]->as<int32_t>();

  return character;
}

int32_t Character::id() const
{
  return _id;
}

void Character::crouch()
{
  sprite->set_animation("Crouch", true);
}

void Character::think(std::shared_ptr<Game> game)
{
  uint32_t delta_ms = this->think_delta_ms();
  auto& vel = this->velocity();

  // External forces, like gravity
  Rect fall_check = this->want_position_y();
  fall_check.y += game->gravity * fall_time_ms / 1000.0;
  if (game->entity_can_move_to(static_cast<Entity*>(this), fall_check)) {
    vel.y += game->gravity * fall_time_ms / 1000.0;
    fall_time_ms += delta_ms;
    falling = true;
  } else {
    if (falling) {
      vel.y = 0;
    }
    falling = false;
    fall_time_ms = 0;
  }

  Rect want_x = this->want_position_x();
  Rect want_y = this->want_position_y();

  if (game->entity_can_move_to(static_cast<Entity*>(this), want_x)) {
    if (vel.x < 0) {
      sprite->flip_x = false;
    } else if (vel.x > 0) {
      sprite->flip_x = true;
    }
    sprite->x = want_x.x;
    auto& pos = this->position();
    pos.x = want_x.x;
  }

  if (game->entity_can_move_to(static_cast<Entity*>(this), want_y)) {
    sprite->y = want_y.y;
    auto& pos = this->position();
    pos.y = want_y.y;
  }

  sprite->render(game->renderer);
  this->reset_think_delta();
}

void Character::walk(float scale)
{
  auto& vel = this->velocity();
  vel.x = scale * walk_speed;
  sprite->set_animation("Walk");
}

void Character::run(float scale)
{
  auto& vel = this->velocity();
  vel.x = scale * run_speed;
  sprite->set_animation("Run");
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
    this->walk(state.x/ 0.5f);
  } else if (mag_x >= 0.5f) {
    this->run(state.x / 1.0f);
  }

  return true;
}


bool Character::on_button_down(const ControllerState& state)
{
  if (state.button == Button::a && this->velocity().y >= 0) {
    std::clog << "Jumping!\n";
    this->velocity().y -= this->jump_vel;
    sprite->set_animation("Jump");
  }
  return true;
}

void Character::attach_controller(std::shared_ptr<Controller> controller_)
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
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = sprite->x;
  box.y = (sprite->y);
  box.w = (current_frame.w * sprite->scale);
  box.h = (current_frame.h * sprite->scale);
  return box;
}

} // namespace raptr
