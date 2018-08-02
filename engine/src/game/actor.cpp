#include <iostream>
#include <memory>
#include <string>
#include <map>

#pragma warning(disable : 4996)
#include <toml/toml.h>
#pragma warning(default : 4996)

#include <raptr/game/game.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/game/actor.hpp>
#include <raptr/common/logging.hpp>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr
{
Actor::Actor()
  : Entity()
{
}

std::shared_ptr<Actor> Actor::from_toml(const FileInfo& toml_path)
{
  const auto toml_relative = toml_path.file_relative;
  auto ifs = toml_path.open();
  if (!ifs) {
    return nullptr;
  }

  auto pr = toml::parse(*ifs);
  if (!pr.valid()) {
    logger->error("Failed to parse {} with reason {}", toml_relative, pr.errorReason);
    return nullptr;
  }

  const auto& v = pr.value;

  std::string toml_keys[] = {
    "actor.name",
    "sprite.path",
    "sprite.scale",
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

  const auto sprite_path = dict["sprite.path"]->as<std::string>();
  auto full_sprite_path = toml_path.file_dir / sprite_path;
  if (!fs::exists(full_sprite_path)) {
    full_sprite_path = toml_path.game_root / sprite_path;
    if (!fs::exists(full_sprite_path)) {
      logger->error("{} is not a valid sprite path in {}", sprite_path, toml_relative);
      return nullptr;
    }
  }

  FileInfo sprite_file;
  sprite_file.game_root = toml_path.game_root;
  sprite_file.file_path = full_sprite_path;
  sprite_file.file_relative = sprite_path;
  sprite_file.file_dir = sprite_file.file_path.parent_path();

  std::shared_ptr<Actor> actor(new Actor());
  actor->sprite = Sprite::from_json(sprite_file);
  actor->sprite->scale = dict["sprite.scale"]->as<double>();
  actor->sprite->set_animation("Idle");
  actor->sprite->x = 0;
  actor->sprite->y = 0;

  actor->do_pixel_collision_test = false;
  if (!actor->sprite->collision_frame_lut.empty()) {
    actor->do_pixel_collision_test = true;
  }
  
  actor->is_scripted = false;
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
    actor->lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::io);

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

    actor->lua_script_fileinfo = lua_script_fileinfo;
    actor->lua_script = *script;
    actor->is_scripted = true;
  }

  auto& pos = actor->position_rel();
  auto& vel = actor->velocity_rel();
  auto& acc = actor->acceleration_rel();
  pos.x = 0;
  pos.y = 0;
  vel.x = 0;
  vel.y = 0;
  acc.x = 0;
  acc.y = 0;

  return actor;
}

void Actor::setup_lua_context(sol::state& state)
{
  state.new_usertype<Actor>("Actor",
    sol::base_classes, sol::bases<Entity>()
  );
}

std::vector<Rect> Actor::bbox() const
{
  std::vector<Rect> boxes;
  Rect box;
  auto& current_frame = sprite->current_animation->current_frame();
  auto pos = this->position_abs();
  box.x = pos.x;
  box.y = pos.y;
  box.w = current_frame.w * sprite->scale;
  box.h = current_frame.h * sprite->scale;
  boxes.push_back(box);
  return boxes;
}

void Actor::think(std::shared_ptr<Game>& game)
{
  auto pos = this->position_abs();
  sprite->x = pos.x;
  sprite->y = pos.y;
}

void Actor::render(Renderer* renderer)
{
  sprite->render(renderer);
}

void Actor::serialize(std::vector<NetField>& list)
{
  NetFieldType cls = NetFieldType::Actor;

  // Stop looking at me like that.
  // This macro helps expand our fields into
  // field type, offset into the class, sizeof object, and data
  #define SNF(field) NetFieldMacro(Actor, field)

  NetField states[] = {
    SNF(pos_),
    SNF(vel_),
    SNF(acc_),
  };

  #undef NF

  for (auto state : states) {
    list.push_back(state);
  }
}


bool Actor::deserialize(const std::vector<NetField>& fields)
{
  return true;
}
} // namespace raptr
