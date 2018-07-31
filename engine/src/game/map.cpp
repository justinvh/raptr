#include <sstream>
#include <raptr/game/map.hpp>
#include <picojson.h>
#include <raptr/common/logging.hpp>
#include <SDL_image.h>
#include "raptr/renderer/renderer.hpp"

namespace {
auto logger = raptr::_get_logger(__FILE__);
const uint32_t FLIPPED_HORIZONTALLY_FLAG = 1 << 31;
const uint32_t FLIPPED_VERTICALLY_FLAG = 1 << 30;
const uint32_t FLIPPED_DIAGONALLY_FLAG = 1 << 29;
const uint32_t CLEAR_FLIP = ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);
};


namespace raptr
{

std::shared_ptr<Map> Map::load(const FileInfo& folder)
{
  auto map_json = folder / "map.json";
  auto input = map_json.open();

  if (!input) {
    return nullptr;
  }

  picojson::value doc;
  *input >> doc;

  auto check_ref = [&](const std::string& name, picojson::value& ref) -> void
  {
    if (!ref.contains(name)) {
      std::stringstream ss;
      ss << "Expected '" << name << "', but it was not found.";
      throw std::runtime_error(ss.str());
    }
  };

  auto U = [&](const std::string& name, picojson::value& ref) -> uint32_t
  {
    check_ref(name, ref);
    return static_cast<uint32_t>(ref.get(name).get<double>());
  };

  auto I = [&](const std::string& name, picojson::value& ref) -> int32_t
  {
    check_ref(name, ref);
    return static_cast<int32_t>(ref.get(name).get<double>());
  };

  auto S = [&](const std::string& name, picojson::value& ref) -> std::string
  {
    check_ref(name, ref);
    return ref.get(name).get<std::string>();
  };

  auto B = [&](const std::string& name, picojson::value& ref) -> bool
  {
    check_ref(name, ref);
    return ref.get(name).get<bool>();
  };

  const auto map = std::make_shared<Map>();
  map->height = U("height", doc);
  map->width = U("width", doc);
  map->tile_height = U("tileheight", doc);
  map->tile_width = U("tilewidth", doc);

  auto player_layer_found = false;
  auto layers = doc.get("layers").get<picojson::array>();

  uint32_t max_tile_id = 0;
  for (auto& pico_layer : layers) {
    auto layer_type = S("type", pico_layer);
    if (layer_type != "tilelayer") {
      continue;
    }
    auto layer_data = pico_layer.get("data").get<picojson::array>();
    int k = 0;
    for (auto d : layer_data) {
      k++;
      auto tile_id = static_cast<uint32_t>(d.get<double>());
      auto original = tile_id;
      tile_id &= CLEAR_FLIP;
      if (tile_id > max_tile_id) {
        max_tile_id = tile_id;
      }
    }
    logger->info("Layer data size is {}", k);
  }
  map->tilemap.resize(max_tile_id + 1);

  auto tileset_data = doc.get("tilesets").get<picojson::array>();
  for (auto& tileset : tileset_data) {
    auto tile_off = U("firstgid", tileset);
    auto source_json = folder / S("source", tileset);
    if (tile_off > max_tile_id) {
      logger->warn("Tileset {} is being excluded because there are no tiles used from it.", source_json);
      continue;
    }

    picojson::value source_doc;
    auto source_input = source_json.open();

    if (!source_input) {
      logger->error("Tileset at {} does not exist", source_json);
      return nullptr;
    }

    *source_input >> source_doc;
    auto source_tiles = source_doc.get("tiles").get<picojson::object>();
    for (auto& source_tile : source_tiles) {
      auto key = std::stoi(source_tile.first);
      auto source_tile_params = source_tile.second;
      auto source_tile_image = source_json.from_current_dir(S("image", source_tile_params));

      std::string source_tile_type = "Non-Collidable";
      if (source_tile_params.contains("type")) {
        source_tile_type = S("type", source_tile_params);
      }

      auto source_tile_image_path = source_tile_image.file_path.string();
      SDL_Surface* surface = IMG_Load(source_tile_image_path.c_str());
      if (!surface) {
        logger->error("Tileset at {} could not load {}", source_json, source_tile_image);
        return nullptr;
      }
      auto& tilemap = map->tilemap[tile_off + key];
      tilemap.surface.reset(surface, SDLDeleter());
      tilemap.type = source_tile_type;
      tilemap.src.x = 0;
      tilemap.src.y = 0;
      tilemap.src.w = surface->w;
      tilemap.src.h = surface->h;
    }
  }

  for (auto& pico_layer : layers) {
    auto layer_type = S("type", pico_layer);
    if (layer_type != "objectgroup") {
      continue;
    }
    auto objects = pico_layer.get("objects").get<picojson::array>();
    for (auto& object : objects) {
      auto type = S("type", object);
      auto properties = object.get("properties");
      if (type == "Parallax") {
        auto script_raw = S("script", properties);
        auto script_path = folder.from_root(script_raw);
        auto is_background = B("is_background", properties);
        auto parallax = Parallax::from_toml(script_path);
        auto width = U("width", object);
        auto height = U("height", object);
        auto x = U("x", object);
        auto y = U("y", object);
        if (!parallax) {
          logger->error("Parallax at {} could not be loaded", script_raw);
          return nullptr;
        }
        parallax->dst.x = x;
        parallax->dst.y = y;
        parallax->dst.w = width;
        parallax->dst.h = height;
        if (is_background) {
          map->parallax_bg.push_back(parallax);
        } else {
          map->parallax_fg.push_back(parallax);
        }
      }
    }
  }

  bool is_foreground = true;
  for (auto& pico_layer : layers) {
    Layer layer;
    auto layer_type = S("type", pico_layer);
    if (layer_type != "tilelayer") {
      continue;
    }
    layer.height = U("height", pico_layer);
    layer.width = U("width", pico_layer);
    layer.name = S("name", pico_layer);
    layer.x = I("x", pico_layer);
    layer.y = I("y", pico_layer);
    if (layer.name == "Player") {
      is_foreground = false;
      player_layer_found = true;
    }

    layer.is_foreground = is_foreground;

    auto layer_data = pico_layer.get("data").get<picojson::array>();
    for (auto d : layer_data) {
      auto tile_id = static_cast<uint32_t>(d.get<double>());
      auto tilemap_idx = tile_id & CLEAR_FLIP;
      layer.data.push_back(tile_id);
      layer.tile_table.push_back(tilemap_idx);
    }

    for (uint32_t y = 0; y < layer.height; ++y) {
      for (uint32_t x = 0; x < layer.width; ++x) {
        uint32_t tile_offset = y * layer.width + x;
        uint32_t tile_index = layer.data[tile_offset];
        if (tile_index == 0) {
          continue;
        }

        uint32_t tilemap_idx = tile_index & CLEAR_FLIP;

        auto& tile = map->tilemap[tilemap_idx];
        if (!tile.surface) {
          logger->error("Tile surface was not allocated!");
          DebugBreak();
        }

        LayerTile l;
        l.dst.x = (layer.x + x) * map->tile_width;
        l.dst.y = (layer.height - y - layer.y - 1) * map->tile_height;
        l.dst.w = tile.src.w;
        l.dst.h = tile.src.h;
        l.flip_x = tile_index & FLIPPED_HORIZONTALLY_FLAG;
        l.flip_y = tile_index & FLIPPED_VERTICALLY_FLAG;
        l.rotation_deg = 0.0;
        if (tile_index & FLIPPED_DIAGONALLY_FLAG && l.flip_x) {
          l.rotation_deg = 90.0;
        } else if (tile_index & FLIPPED_DIAGONALLY_FLAG) {
          l.rotation_deg = -90.0;
        }

        l.tile = &map->tilemap[tilemap_idx];
        layer.renderable.emplace_back(l);
      }
    }

    map->layers.push_back(layer);
  }

  if (!player_layer_found) {
    logger->error("Expected {} to have a 'Player' layer, but it does not.", map_json.file_path);
    return nullptr;
  }

  map->tilemap_texture_allocated = false;

  return map;
}

void Map::render_layer(Renderer* renderer, const Layer& layer)
{
  for (auto& l : layer.renderable) {
    Tile* tile = l.tile;
    SDL_Rect dst = l.dst;
    auto texture = tile->texture;
    if (!texture) {
      DebugBreak();
    }
    renderer->add_texture(texture, tile->src, dst, l.rotation_deg, l.flip_x, l.flip_y, false, layer.is_foreground);
  }
}

bool Map::intersects(const Entity* other) const
{
  if (!other->collidable) {
    return false;
  }

  if (other->do_pixel_collision_test) {
    for (auto& other_box : other->bbox()) {
      if (this->intersect_slow(other, other_box)) {
        return true;
      }
    }
    return false;
  }

  for (auto& other_box : other->bbox()) {
    if (this->intersects(other_box)) {
      return true;
    }
  }

  return false;
}

bool Map::intersects(const Rect& bbox) const
{
  return this->intersect_slow(bbox);
}

bool Map::intersects(const Entity* other, const Rect& bbox) const
{
  if (!other->collidable) {
    return false;
  }

  if (other->do_pixel_collision_test) {
    return this->intersect_slow(other, bbox);
  }

  return this->intersects(bbox);
}

bool Map::intersect_slow(const Entity* other, const Rect& bbox) const
{
  // Is there a tile in this map that would occupy X/Y
  for (auto& layer : layers) {
    const int32_t check_x1 = (bbox.x - (layer.x * tile_width)) / tile_width;
    const int32_t check_x2 = ((bbox.x + bbox.w) - (layer.x * tile_width)) / tile_width;
    const int32_t check_y1 = ((layer.height - layer.y) * tile_height - (bbox.y + bbox.h)) / tile_height;
    const int32_t check_y2 = ((layer.height - layer.y) * tile_height - bbox.y) / tile_height;

    if (check_x1 < 0 || check_x2 >= layer.width) {
      continue;
    }

    if (check_y1 < 0 || check_y2 >= layer.height) {
      continue;
    }

    for (int32_t check_y = check_y1; check_y <= check_y2; ++check_y) {
      for (int32_t check_x = check_x1; check_x <= check_x2; ++check_x) {
        uint32_t idx = (check_y * layer.width + check_x);
        if (layer.tile_table[idx] == 0) {
          continue;
        }
        auto& tile = this->tilemap[layer.tile_table[idx]];
        if (tile.type == "Collidable") {
          return true;
        }
      }
    }
  }
  return false;
}


bool Map::intersect_slow(const Rect& other_box) const
{
  return false;
}


void Map::render(Renderer* renderer)
{
  if (!tilemap_texture_allocated) {
    for (auto& tile : tilemap) {
      if (!tile.surface) {
        continue;
      }
      tile.texture.reset(
        renderer->create_texture(tile.surface),
        SDLDeleter());
    }

    for (auto& parallax : parallax_bg) {
      renderer->add_background(parallax);
    }

    for (auto& parallax : parallax_fg) {
      renderer->add_foreground(parallax);
    }

    tilemap_texture_allocated = true;
  }

  for (const auto& layer : layers) {
    this->render_layer(renderer, layer);
  }
}
  
}