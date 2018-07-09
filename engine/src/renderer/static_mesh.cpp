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

namespace { auto logger = raptr::_get_logger(__FILE__); };

namespace raptr {

StaticMesh::StaticMesh()
  : Entity()
{
}

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

  staticmesh->do_pixel_collision_test = false;
  if (staticmesh->sprite->has_animation("Collision")) {
    staticmesh->collision_frame = &staticmesh->sprite->animations["Collision"].frames[0];
    staticmesh->do_pixel_collision_test = true;
  }

  auto& pos = staticmesh->position();
  auto& vel = staticmesh->velocity();
  auto& acc = staticmesh->acceleration();
  pos.x = 0; pos.y = 0;
  vel.x = 0; vel.y = 0;
  acc.x = 0; acc.y = 0;

  return staticmesh;
}


std::vector<Rect> StaticMesh::bbox() const
{
  std::vector<Rect> boxes;
  Rect box;
  auto& current_frame = sprite->current_animation->current_frame();
  auto& pos = this->position();
  box.x = pos.x;
  box.y = pos.y;
  box.w = current_frame.w * sprite->scale;
  box.h = current_frame.h * sprite->scale;
  boxes.push_back(box);
  return boxes;
}

void StaticMesh::think(std::shared_ptr<Game>& game)
{
  auto& pos = position();
  sprite->x = pos.x;
  sprite->y = pos.y;
}

void StaticMesh::render(Renderer* renderer)
{
  sprite->render(renderer);
}

void StaticMesh::serialize(std::vector<NetField>& list)
{
  NetFieldType cls = NetFieldType::StaticMesh;

  // Stop looking at me like that.
  // This macro helps expand our fields into
  // field type, offset into the class, sizeof object, and data
  #define SNF(field) NetFieldMacro(StaticMesh, field)

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


bool StaticMesh::deserialize(const std::vector<NetField>& fields)
{
  return true;
}

} // namespace raptr
