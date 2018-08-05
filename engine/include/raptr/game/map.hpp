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
#include <array>

#include <SDL.h>
#include <SDL_surface.h>

#include <raptr/game/entity.hpp>
#include <raptr/common/filesystem.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/parallax.hpp>

namespace raptr
{

class Game;

struct Tile
{
  bool loaded;
  std::shared_ptr<SDL_Surface> surface;
  std::shared_ptr<SDL_Texture> texture;
  std::shared_ptr<Sprite> sprite;
  std::string type;
  SDL_Rect src;
};

struct LayerTile
{
  Tile* tile;
  std::shared_ptr<Sprite> sprite;
  SDL_Rect dst;
  bool flip_x;
  bool flip_y;
  float rotation_deg;
  uint32_t index;
};

struct Layer
{
  std::string name;
  std::vector<uint32_t> data;
  std::vector<uint32_t> tile_table;
  std::vector<LayerTile> renderable;
  std::map<uint32_t, LayerTile> layer_tile_lut;
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
  void think(std::shared_ptr<Game>& game);
  LayerTile* intersects(const Entity* entity, const std::string& tile_type = "Collidable");
  LayerTile* intersects(const Entity* entity, const Rect& bbox, const std::string& tile_type = "Collidable");
  LayerTile* intersects(const Rect& bbox, const std::string& tile_type = "Collidable");
  bool intersect_precise(const LayerTile* tile, const Layer& layer,
                         int32_t check_x, int32_t check_y,
                         const Entity* other, const Rect& this_bbox,
                         bool use_entity_collision_frame);
  LayerTile* intersect_slow(const Entity* other, const Rect& this_bbox, const std::string& tile_type = "Collidable");
  LayerTile* intersect_slow(const Rect& this_bbox, const std::string& tile_type = "Collidable");

public:
  std::string name;
  Rect player_spawn;
  bool tilemap_texture_allocated;
  std::vector<std::shared_ptr<Parallax>> parallax_bg, parallax_fg;
  std::vector<std::array<unsigned char, 16>> should_kill;
  std::vector<Layer> layers;
  std::vector<Tile> tilemap;
  uint32_t width, height;
  uint32_t tile_width, tile_height;
};

} // namespace raptr
