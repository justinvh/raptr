/*!
  \file map.hpp
  This file provides the interaction and loading of meshes,
  triggers, and other sprites for a world
*/
#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_surface.h>

#include <raptr/game/entity.hpp>
#include <raptr/common/filesystem.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/parallax.hpp>

namespace raptr
{

struct Tile
{
  bool loaded;
  std::shared_ptr<SDL_Surface> surface;
  std::shared_ptr<SDL_Texture> texture;
  std::string type;
  SDL_Rect src;
};

struct LayerTile
{
  Tile* tile;
  SDL_Rect dst;
  bool flip_x;
  bool flip_y;
  float rotation_deg;
};

struct Layer
{
  std::string name;
  std::vector<uint32_t> data;
  std::vector<uint32_t> tile_table;
  std::vector<LayerTile> renderable;
  int32_t x, y;
  uint32_t width, height;
  bool is_foreground;
};

class Map : public RenderInterface
{
public:
  static std::shared_ptr<Map> load(const FileInfo& folder);
  void render(Renderer* renderer) override;
  void render_layer(Renderer* renderer, const Layer& layer);
  bool intersects(const Entity* entity) const;
  bool intersects(const Entity* entity, const Rect& bbox) const;
  bool intersects(const Rect& bbox) const;
  bool intersect_slow(const Entity* other, const Rect& this_bbox) const;
  bool intersect_slow(const Rect& this_bbox) const;

public:
  std::string name;
  bool tilemap_texture_allocated;
  std::vector<std::shared_ptr<Parallax>> parallax_bg, parallax_fg;
  std::vector<Layer> layers;
  std::vector<Tile> tilemap;
  uint32_t width, height;
  uint32_t tile_width, tile_height;
};
} // namespace raptr
