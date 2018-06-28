#include <iostream>
#include <memory>
#include <string>
#include <map>

#pragma warning(disable : 4996)
#include <toml/toml.h>
#pragma warning(default : 4996)

#include <raptr/game/game.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/renderer/static_mesh.hpp>
#include <raptr/common/logging.hpp>

macro_enable_logger();

namespace raptr {

std::shared_ptr<StaticMesh> StaticMesh::from_toml(const FileInfo& toml_path)
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
    "staticmesh.name",
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

  FileInfo sprite_file;
  sprite_file.game_root = toml_path.game_root;
  sprite_file.file_path = toml_path.game_root / sprite_path;
  sprite_file.file_relative = sprite_path;
  sprite_file.file_dir = sprite_file.file_path.parent_path();

  std::shared_ptr<StaticMesh> staticmesh(new StaticMesh());
  staticmesh->sprite = Sprite::from_json(sprite_file);
  staticmesh->sprite->scale = dict["sprite.scale"]->as<double>();
  staticmesh->sprite->set_animation("Idle");
  staticmesh->sprite->x = 0;
  staticmesh->sprite->y = 0;

  return staticmesh;
}

int32_t StaticMesh::id() const
{
  return _id;
}

bool StaticMesh::intersects(const Entity* other) const
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

Rect StaticMesh::bbox() const
{
  Rect box;
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = static_cast<int32_t>(sprite->x);
  box.y = static_cast<int32_t>(sprite->y);
  box.w = current_frame.w * sprite->scale;
  box.h = current_frame.h * sprite->scale;
  return box;
}


void StaticMesh::think(std::shared_ptr<Game> game)
{
  sprite->render(game->renderer);
}

} // namespace raptr
