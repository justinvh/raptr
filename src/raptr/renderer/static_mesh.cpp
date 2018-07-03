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

bool StaticMesh::intersects(const Entity* other) const
{
  if (other->id() == this->id()) {
    return false;
  }

  for (auto& self_box : this->bbox()) {
    for (auto& other_box : other->bbox()) {
      if (other->intersects(self_box) && this->intersects(other_box)) {
        return true;
      }
    }
  }

  return false;
}

bool StaticMesh::intersects(const Rect& bbox) const
{
  if (this->do_pixel_collision_test) {
    return this->intersect_slow(bbox);
  } else {
    return this->intersect_fast(bbox);
  }
}

bool StaticMesh::intersect_slow(const Rect& other_box) const
{
  const auto& surface = sprite->surface;
  const uint8_t* pixels = reinterpret_cast<uint8_t*>(surface->pixels);
  int32_t bpp = surface->format->BytesPerPixel;

  auto& pos = this->position();
  auto& frame = collision_frame;

  // x0 position
  int32_t x0_min = static_cast<int32_t>(std::max(other_box.x, pos.x));
  int32_t x0_max = static_cast<int32_t>(std::min(other_box.x + other_box.w,
                                                 pos.x + frame->w - 1));

  // y0 position
  int32_t y0_min = static_cast<int32_t>(std::max(other_box.y, pos.y));
  int32_t y0_max = static_cast<int32_t>(std::min(other_box.y + other_box.h,
                                                 pos.y + frame->h - 1));

  for (int32_t x0 = x0_min; x0 <= x0_max; ++x0) {
    for (int32_t y0 = y0_min; y0 <= y0_max; ++y0) {
      int32_t idx = ((frame->y + y0) * surface->pitch) + (x0 + frame->x) * bpp;
      const uint8_t* px = pixels + idx;
      if (*px > 0) {
        return true;
      }
    }
  }

  return false;
}

bool StaticMesh::intersect_fast(const Rect& other_box) const
{
  const auto& self_boxes = this->bbox();

  for (const auto& self_box : self_boxes) {
    if (SDL_HasIntersection(&self_box, &other_box)) {
      return true;
    }
  }

  return false;
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
  sprite->render(game->renderer);
}

} // namespace raptr
