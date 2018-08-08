#include <sstream>
#include <picojson.h>
#include <SDL_image.h>

#include <raptr/game/character.hpp>
#include <raptr/game/map.hpp>
#include <raptr/game/game.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/ui/dialog.hpp>

namespace 
{
auto logger = raptr::_get_logger(__FILE__);
const uint32_t FLIPPED_HORIZONTALLY_FLAG = 1 << 31;
const uint32_t FLIPPED_VERTICALLY_FLAG = 1 << 30;
const uint32_t FLIPPED_DIAGONALLY_FLAG = 1 << 29;
const uint32_t CLEAR_FLIP = ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);
};

namespace raptr
{

namespace parser {

auto check_ref = [&](const std::string& name, const picojson::value& ref) -> void
{
  if (!ref.contains(name)) {
    std::stringstream ss;
    ss << "Expected '" << name << "', but it was not found.";
    throw std::runtime_error(ss.str());
  }
};

auto U = [&](const std::string& name, const picojson::value& ref) -> uint32_t
{
  check_ref(name, ref);
  return static_cast<uint32_t>(ref.get(name).get<double>());
};

auto I = [&](const std::string& name, const picojson::value& ref) -> int32_t
{
  check_ref(name, ref);
  return static_cast<int32_t>(ref.get(name).get<double>());
};

auto S = [&](const std::string& name, const picojson::value& ref) -> std::string
{
  check_ref(name, ref);
  return ref.get(name).get<std::string>();
};

auto B = [&](const std::string& name, const picojson::value& ref) -> bool
{
  check_ref(name, ref);
  return ref.get(name).get<bool>();
};

uint32_t find_max_tile_id(const picojson::array& layers)
{
  uint32_t max_tile_id = 0;
  for (auto& pico_layer : layers) {
    auto layer_type = S("type", pico_layer);
    if (layer_type == "tilelayer") {
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
    } else if (layer_type == "objectgroup") {
      auto objects = pico_layer.get("objects").get<picojson::array>();
      for (auto& object : objects) {
        if (!object.contains("gid")) {
          continue;
        }
        auto tile_id = U("id", object);
        tile_id &= CLEAR_FLIP;
        if (tile_id > max_tile_id) {
          max_tile_id = tile_id;
        }
      }
    }
  }
  return max_tile_id;
}

bool load_tileset(const picojson::value& tileset,
                  const FileInfo& folder,
                  const std::shared_ptr<Map>& map,
                  uint32_t max_tile_id)

{
  auto tile_off = U("firstgid", tileset);
  auto source_json = folder / S("source", tileset);
  if (tile_off > max_tile_id) {
    logger->warn("Tileset {} is being excluded because there are no tiles used from it.", source_json);
    return true;
  }

  picojson::value source_doc;
  auto source_input = source_json.open();

  if (!source_input) {
    logger->error("Tileset at {} does not exist", source_json);
    return false;
  }

  *source_input >> source_doc;
  auto source_tiles = source_doc.get("tiles").get<picojson::object>();
  picojson::object tile_properties;

  if (source_doc.contains("tileproperties")) {
    tile_properties = source_doc.get("tileproperties").get<picojson::object>();
  }

  for (auto& source_tile : source_tiles) {
    auto key = std::stoi(source_tile.first);
    auto source_tile_params = source_tile.second;
    auto source_tile_image = source_json.from_current_dir(S("image", source_tile_params));

    if (tile_off + key > max_tile_id) {
      logger->debug("Ignoring tile {} because it is not used", source_tile_image);
      continue;
    }

    std::string source_tile_type = "Non-Collidable";
    if (source_tile_params.contains("type")) {
      source_tile_type = S("type", source_tile_params);
    }

    auto& tilemap = map->tilemap[tile_off + key];
    tilemap.type = source_tile_type;
    tilemap.src.x = 0;
    tilemap.src.y = 0;
    tilemap.src.w = 0;
    tilemap.src.h = 0;

    std::shared_ptr<Sprite> sprite = nullptr;
    auto has_properties = tile_properties.find(source_tile.first);
    if (has_properties != tile_properties.end()) {
      auto properties = has_properties->second.get<picojson::object>();
      auto has_animation = properties.find("animation");
      if (has_animation != properties.end()) {
        const auto animation = has_animation->second.get<std::string>();
        const auto animation_path = folder.from_root(animation);
        sprite = Sprite::from_json(animation_path);
        if (!sprite) {
          logger->error("Failed to load sprite: {}", animation_path);
          return false;
        }
      }
      tilemap.sprite = sprite;
    } else {
      auto source_tile_image_path = source_tile_image.file_path.string();
      SDL_Surface* surface = IMG_Load(source_tile_image_path.c_str());
      if (!surface) {
        logger->error("Tileset at {} could not load {}", source_json, source_tile_image);
        return false;
      }
      tilemap.surface.reset(surface, SDLDeleter());
      tilemap.src.w = surface->w;
      tilemap.src.h = surface->h;
    }
  }
  return true;
}

bool load_parallax(const picojson::value& object, 
                   const FileInfo& folder,
                   const std::shared_ptr<Map>& map)
{
  auto properties = object.get("properties");
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
    return false;
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
  return true;
}

bool load_dialog(const picojson::value& layer,
                 const picojson::value& object, 
                 const FileInfo& folder,
                 const std::shared_ptr<Map>& map)
{
  auto properties = object.get("properties");
  auto sprite_path = folder.from_root(S("sprite", properties));
  auto speaker = S("speaker", properties);
  auto expression = S("expression", properties);
  auto name = S("name", properties);
  auto text = S("text", properties);

  auto sprite = Sprite::from_json(sprite_path);
  if (!sprite) {
    return false;
  }

  auto dialog = Dialog::from_easy_params(folder, speaker, expression, name, text);

  if (!dialog) {
    logger->error("Dialog could not be loaded!");
    return false;
  }

  LayerTile obj;
  obj.sprite = sprite;
  obj.dialog = dialog;
  obj.type = "Interactive";

  auto width = U("width", object);
  auto height = U("height", object);
  auto x = U("x", object);
  auto y = U("y", object);

  obj.dst.x = x;
  obj.dst.y = (map->height * map->tile_height - y);
  obj.sprite->x = obj.dst.x;
  obj.sprite->y = obj.dst.y;
  obj.dst.w = width;
  obj.dst.h = height;
  obj.flip_x = false;
  obj.flip_y = false;

  map->objects.push_back(std::move(obj));
  return true;
}

bool load_lua_script(const picojson::value& layer,
                     const picojson::value& object, 
                     const FileInfo& folder,
                     const std::shared_ptr<Map>& map)
{
  auto properties = object.get("properties");
  auto script = S("script", properties);
  auto sprite_path = folder.from_root(S("sprite", properties));

  auto sprite = Sprite::from_json(sprite_path);
  if (!sprite) {
    return false;
  }

  LayerTile obj;
  obj.script = script;
  obj.sprite = sprite;
  obj.type = "Interactive";

  auto width = U("width", object);
  auto height = U("height", object);
  auto x = U("x", object);
  auto y = U("y", object);

  obj.dst.x = x;
  obj.dst.y = (map->height * map->tile_height - y);
  obj.sprite->x = obj.dst.x;
  obj.sprite->y = obj.dst.y;
  obj.dst.w = width;
  obj.dst.h = height;
  obj.flip_x = false;
  obj.flip_y = false;

  map->objects.push_back(std::move(obj));
  return true;
}

bool load_object(const picojson::value& layer, const picojson::value& object, const FileInfo& folder,
                 const std::shared_ptr<Map>& map)
{
  auto type = S("type", object);
  if (type == "Parallax") {
    return load_parallax(object, folder, map);
  } else if (type == "Dialog") {
    return load_dialog(layer, object, folder, map);
  } else if (type == "LuaScript") {
    return load_lua_script(layer, object, folder, map);
  }

  logger->warn("Unrecognized object type in map: {}", type);
  return true;
}

bool load_tile(uint32_t tilemap_idx, uint32_t tile_index, uint32_t tile_offset,
               int32_t x, int32_t y, Layer& layer, const std::shared_ptr<Map>& map)
{
  auto& tile = map->tilemap[tilemap_idx];
  if (!tile.surface && !tile.sprite) {
    logger->error("Tile surface was not allocated!");
    DebugBreak();
    return false;
  }

  LayerTile l;
  l.index = tile_index;
  l.dst.x = (layer.x + x) * map->tile_width;
  l.dst.y = (layer.height - y - layer.y - 1) * map->tile_height;
  l.dst.w = tile.src.w;
  l.dst.h = tile.src.h;
  l.flip_x = false;
  l.flip_y = false;
  l.flip_x = tile_index & FLIPPED_HORIZONTALLY_FLAG;
  l.flip_y = tile_index & FLIPPED_VERTICALLY_FLAG;
  l.rotation_deg = 0.0;
  if (tile_index & FLIPPED_DIAGONALLY_FLAG) {
    l.rotation_deg = 90.0;
  }

  l.tile = &map->tilemap[tilemap_idx];
  l.type = l.tile->type;

  if (l.tile->sprite) {
    l.sprite = l.tile->sprite->clone();
    l.sprite->flip_x = l.flip_x;
    l.sprite->flip_y = l.flip_y;
    l.sprite->rotation_deg = l.rotation_deg;
    l.sprite->x = l.dst.x;
    l.sprite->y = l.dst.y;
  }

  layer.renderable.emplace_back(l);
  layer.layer_tile_lut[tile_offset] = l;
  return true;
}

bool load_tilelayer(const picojson::value& pico_layer, const FileInfo& folder,
                    const std::shared_ptr<Map>& map)
{
  Layer layer;
  layer.height = U("height", pico_layer);
  layer.width = U("width", pico_layer);
  layer.name = S("name", pico_layer);
  layer.x = I("x", pico_layer);
  layer.y = I("y", pico_layer);

  if (layer.name == "Player") {
    auto layer_data = pico_layer.get("data").get<picojson::array>();
    int32_t k = 0;
    for (auto d : layer_data) {
      auto tile_id = static_cast<uint32_t>(d.get<double>());
      auto tilemap_idx = tile_id & CLEAR_FLIP;
      if (tilemap_idx == 0) {
        ++k;
        continue;
      }
      map->player_spawn.x = (layer.x + (k % layer.width)) * map->tile_width;
      map->player_spawn.y = layer.height * map->tile_height - (layer.y + k / layer.width + 1) * map->tile_height;
      break;
    }
    return true;
  }

  layer.is_foreground = false;

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
      if (!load_tile(tilemap_idx, tile_index, tile_offset, x, y, layer, map)) {
        return false;
      }
    }
  }

  map->layers.push_back(layer);
  return true;
}

}

std::shared_ptr<Map> Map::load(const FileInfo& folder)
{
  auto map_json = folder / "map.json";
  auto input = map_json.open();

  if (!input) {
    return nullptr;
  }

  picojson::value doc;
  *input >> doc;

  // These are small functions for unwrapping types
  using parser::U;
  using parser::S;
  using parser::I;
  using parser::B;
 
  // These are the base criteria for our map and define a quick and
  // efficient way for navigating the map for collisions
  const auto map = std::make_shared<Map>();
  map->height = U("height", doc);
  map->width = U("width", doc);
  map->tile_height = U("tileheight", doc);
  map->tile_width = U("tilewidth", doc);
  map->tilemap_texture_allocated = false;

  // The layers represent both objects and tiles
  auto layers = doc.get("layers").get<picojson::array>();

  // We create a very sparse representation of the tiles by
  // finding the maximum tile id through the entire loaded map
  const auto max_tile_id = parser::find_max_tile_id(layers);
  map->tilemap.resize(max_tile_id + 1);

  // Iterate through each of the tilesets in the map and load
  // them into the map->tilemap property
  auto tileset_data = doc.get("tilesets").get<picojson::array>();
  for (auto& tileset : tileset_data) {
    if (!parser::load_tileset(tileset, folder, map, max_tile_id)) {
      return nullptr;
    }
  }

  // Iterate through each of the layers and load the objects for
  // the map. This will include objects such as Parallax, Dialog, etc.
  for (auto& pico_layer : layers) {
    auto layer_type = S("type", pico_layer);
    if (layer_type != "objectgroup") {
      continue;
    }
    auto objects = pico_layer.get("objects").get<picojson::array>();
    for (auto& object : objects) {
      if (!parser::load_object(pico_layer, object, folder, map)) {
        return nullptr;
      }
    }
  }

  // Iterate through each of the layers and load the tiles for
  // the map. These are fixed at a tile_width / tile_height grid and
  // have limited (or rather well defined) actions in the world
  bool is_foreground = true;
  for (auto& pico_layer : layers) {
    auto layer_type = S("type", pico_layer);
    if (layer_type != "tilelayer") {
      continue;
    }
    if (!parser::load_tilelayer(pico_layer, folder, map)) {
      logger->error("Failed to load tile layer");
      return nullptr;
    }
  }

  return map;
}

void Map::render_layer(Renderer* renderer, const Layer& layer)
{
  for (auto& l : layer.renderable) {
    if (l.sprite) {
      l.sprite->render(renderer);
      continue;
    }

    Tile* tile = l.tile;
    auto texture = tile->texture;
    renderer->add_texture(texture, tile->src, l.dst, l.rotation_deg, l.flip_x, l.flip_y, false, layer.is_foreground);
  }
}

LayerTile* Map::intersects(const Entity* other, const std::string& tile_type) 
{
  if (!other->collidable) {
    return nullptr;
  }

  auto other_box = other->bbox();

  if (other->do_pixel_collision_test) {
    auto result = this->intersect_slow(other, other_box, tile_type);
    if (result) {
      return result;
    }
    return nullptr;
  }

  auto result = this->intersects(other_box, tile_type);
  if (result) {
    return result;
  }

  return nullptr;
}

LayerTile* Map::intersects(const Rect& bbox, const std::string& tile_type)
{
  return this->intersect_slow(bbox, tile_type);
}

LayerTile* Map::intersects(const Entity* other, const Rect& bbox, const std::string& tile_type)
{
  if (!other->collidable) {
    return nullptr;
  }

  if (other->do_pixel_collision_test) {
    return this->intersect_slow(other, bbox, tile_type);
  }

  return this->intersects(bbox, tile_type);
}

bool Map::intersect_precise(const LayerTile* layer_tile, 
                            int32_t tx, int32_t ty,
                            const Entity* other, const Rect& bbox,
                            bool use_entity_collision_frame)
{
  auto& tile = layer_tile->tile;
  std::shared_ptr<SDL_Surface> this_surface = nullptr;

  SDL_Rect this_offset = {0, 0, 0, 0};
  if (layer_tile->sprite) {
    this_surface = layer_tile->sprite->surface;
    const auto& this_frame = layer_tile->sprite->current_collision->current_frame();
    this_offset.x = this_frame.x;
    this_offset.y = this_frame.y;
    this_offset.w = this_frame.w;
    this_offset.h = this_frame.h;
  } else {
    this_surface = tile->surface;
    this_offset.x = 0;
    this_offset.y = 0;
    this_offset.w = this_surface->w;
    this_offset.h = this_surface->h;
  }

  uint8_t* this_pixels = reinterpret_cast<uint8_t*>(this_surface->pixels);
  const int32_t this_bpp = this_surface->format->BytesPerPixel;

  auto& other_sprite = other->sprite;
  const auto& other_surface = other_sprite->surface;
  const uint8_t* other_pixels = reinterpret_cast<uint8_t*>(other_surface->pixels);
  const int32_t other_bpp = other_surface->format->BytesPerPixel;
  
  const auto other_pos = bbox;
  AnimationFrame* other_frame = nullptr;
  if (use_entity_collision_frame) {
    other_frame = other->collision_frame();
  } else {
    other_frame = &other_sprite->current_animation->current_frame();
  }

  SDL_Rect other_offset = {
    other_frame->x,
    other_frame->y,
    other_frame->w,
    other_frame->h
  };

  // Bounding box of tile relative to the layer
  double ax0 = tx;
  double ax1 = tx + this_offset.w - 1;
  double ay0 = ty;
  double ay1 = ty + this_offset.h - 1;

  // Bounding box of entity relative to the layer
  double bx0 = other_pos.x;
  double bx1 = other_pos.x + other_offset.w - 1;
  double by0 = other_pos.y;
  double by1 = other_pos.y + other_offset.h - 1;

  // Overlap of the two relative to the layer
  auto cx0 = std::max(ax0, bx0);
  auto cx1 = std::min(ax1, bx1);
  auto cy0 = std::max(ay0, by0);
  auto cy1 = std::min(ay1, by1);

  // This shouldn't happen
  if (cx1 < cx0 || cy1 < cy0) {
    return nullptr;
  }

  // Pixel range for tile
  auto tx0 = static_cast<int32_t>(cx0 - ax0);
  auto ty0 = static_cast<int32_t>(cy0 - ay0);

  // Other pixel offsets
  auto ox0 = static_cast<int32_t>(cx0 - bx0);
  auto oy0 = static_cast<int32_t>(cy0 - by0);

  auto th = static_cast<int32_t>(this_offset.h - 1);
  auto tw = static_cast<int32_t>(this_offset.w - 1);
  auto oh = static_cast<int32_t>(other_offset.h - 1);
  auto ow = static_cast<int32_t>(other_offset.w - 1);

  auto w = static_cast<int32_t>(cx1 - cx0);
  auto h = static_cast<int32_t>(cy1 - cy0);

  for (int32_t y = 0; y < h; ++y) {
    int32_t tile_y_px = this_offset.y + th - (ty0 + y);
    if (layer_tile->flip_y) {
      tile_y_px = this_offset.y + ty0 + y;
    }

    int32_t sprite_y_px = other_offset.y + oh - (oy0 + y);
    if (other_sprite->flip_y) {
      sprite_y_px = other_offset.y + oy0 + y;
    }

    for (int32_t x = 0; x < w; ++x) {
      int32_t tile_x_px = this_offset.x + tx0 + x;
      if (layer_tile->flip_x) {
        tile_x_px = this_offset.x + tw - (tx0 + x);
      }

      int32_t sprite_x_px = other_offset.x + ox0 + x;
      if (other_sprite->flip_x) {
        sprite_x_px = other_offset.x + ow - (ox0 + x);
      }

      int32_t this_idx = tile_y_px * this_surface->pitch + tile_x_px * this_bpp;
      int32_t other_idx = sprite_y_px * other_surface->pitch + sprite_x_px * other_bpp;
      uint8_t* this_px = this_pixels + this_idx;
      const uint8_t* other_px = other_pixels + other_idx;
      if (*this_px > 0 && *other_px > 0) {
        return true;
      }
    }
  }

  return false;
}

void Map::activate_dialog(Entity* activator, LayerTile* tile)
{
  if (!activator->is_player()) {
    return;
  }
  auto character = dynamic_cast<Character*>(activator);
  active_dialog = tile->dialog;
  active_dialog->start();
  active_dialog->attach_controller(character->controller);
}

void Map::activate_tile(std::shared_ptr<Game>& game, Entity* activator, LayerTile* tile)
{
  if (tile->dialog) {
    this->activate_dialog(activator, tile);
  } else if (!tile->script.empty()) {
    game->lua.safe_script(tile->script);
  }
}

LayerTile* Map::intersect_slow(const Entity* other, const Rect& bbox, const std::string& type)
{
  bool use_collision = (type != "Death");

  // Is there a tile in this map that would occupy X/Y
  for (auto& layer : layers) {
    const int32_t x_off = layer.x * tile_width;
    const int32_t y_off = layer.y * tile_height;
    int32_t left = (bbox.x - x_off + 1) / tile_width;
    int32_t right = ((bbox.x + bbox.w + 1) - x_off) / tile_width;
    int32_t bottom = (bbox.y - y_off + 1) / tile_height;
    int32_t top = ((bbox.y + bbox.h + 1) - y_off) / tile_height;

    Rect bbox_rel;
    bbox_rel.x = (bbox.x - x_off);
    bbox_rel.y = (bbox.y - y_off);
    bbox_rel.w = bbox.w;
    bbox_rel.h = bbox.h;

    if (right < 0 || left >= layer.width ||
        top < 0 || bottom >= layer.height) 
    {
      return nullptr;
    }

    left = std::max(0, left);
    right = std::min(static_cast<int32_t>(layer.width) - 1, right);
    top = std::min(static_cast<int32_t>(layer.height) - 1, top);
    bottom = std::max(0, bottom);

    for (int32_t y = bottom; y <= top; ++y) {
      for (int32_t x = left; x <= right; ++x) {
        uint32_t idx = ((layer.height - y - 1) * layer.width + x);

        // There is no tile here
        if (layer.tile_table[idx] == 0) {
          continue;
        }

        int32_t tx = x * tile_width;
        int32_t ty = y * tile_height;

        // Check properties of tile type, instead of checking collision type
        auto layer_tile = &layer.layer_tile_lut[idx];
        if (layer_tile->tile->type == type) {
          if (this->intersect_precise(layer_tile, tx, ty, other, bbox_rel, use_collision)) {
            return layer_tile;
          }
        }
      }
    }
  }

  for (size_t i = 0; i < objects.size(); ++i) {
    auto& obj = objects[i];
    int32_t tx = obj.dst.x;
    int32_t ty = obj.dst.y;
    if (obj.type == type) {
      if (this->intersect_precise(&obj, tx, ty, other, bbox, use_collision)) {
        return &objects[i];
      }
    }
  }

  return nullptr;
}

LayerTile* Map::intersect_slow(const Rect& other_box, const std::string& tile_type)
{
  return nullptr;
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

  for (const auto& obj : objects) {
    obj.sprite->render(renderer);
  }

  if (active_dialog) {
    active_dialog->render(renderer);
  }
}

void Map::think(std::shared_ptr<Game>& game)
{
}
  
}