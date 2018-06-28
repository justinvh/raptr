#pragma
#include <iostream>
#include <memory>
#include <string>

#pragma warning(disable : 4996)
#include <toml/toml.h>
#pragma warning(default : 4996)

#include <raptr/game/game.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/renderer/static_mesh.hpp>
#include <raptr/common/logging.hpp>

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

  const toml::Value* staticmesh_section = v.find("staticmesh");
  if (!staticmesh_section) {
    logger->error("{} is missing [staticmesh] section", toml_relative);
    return nullptr;
  }

  const toml::Value* name_value = staticmesh_section->find("name");
  if (!name_value) {
    logger->error("{} is missing staticmesh.name", toml_relative);
    return nullptr;
  }

  const toml::Value* sprite_section = v.find("sprite");
  if (!sprite_section) {
    logger->error("{} is missing [sprite] section", toml_relative);
    return nullptr;
  }

  const toml::Value* path_value = sprite_section->find("path");
  if (!path_value) {
    logger->error("{} is missing sprite.path", toml_relative);
    return nullptr;
  }

  std::string sprite_path = path_value->as<std::string>();
  if (!fs::exists(toml_path.game_root / sprite_path)) {
    logger->error("{} is not a valid sprite path in {}", sprite_path, toml_relative);
  }

  const toml::Value* scale_value = sprite_section->find("scale");
  if (!scale_value) {
    logger->error("{} is missing sprite.scale", toml_relative);
    return nullptr;
  }

  FileInfo sprite_file;
  sprite_file.game_root = toml_path.game_root;
  sprite_file.file_path = toml_path.game_root / sprite_path;
  sprite_file.file_relative = sprite_path;
  sprite_file.file_dir = sprite_file.file_path.parent_path();

  std::shared_ptr<StaticMesh> staticmesh(new StaticMesh());
  staticmesh->sprite = Sprite::from_json(sprite_file);
  staticmesh->sprite->scale = scale_value->as<double>();
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
