#pragma warning(disable : 4996)
#include <toml/toml.h>
#pragma warning(default : 4996)

#include <SDL_image.h>

#include <raptr/common/logging.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/background.hpp>

namespace { auto logger = raptr::_get_logger(__FILE__); }

namespace raptr {

std::shared_ptr<Background> Background::from_toml(const FileInfo& toml_path)
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
    "background.name",
    "layer",
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

  std::shared_ptr<Background> bg(new Background());

  const toml::Array& layer_values = v.find("layer")->as<toml::Array>();
  for (const toml::Value& l : layer_values) {
    Layer layer;

    std::string texture_rel_path = l.get<std::string>("texture");
    uint32_t z_index = l.get<int32_t>("z");

    auto image_path = toml_path.from_root(texture_rel_path);
    if (!fs::exists(image_path.file_path)) {
      logger->error("Background texture at {} does not exist", image_path.file_path);
      return nullptr;
    }

    std::string image_cpath(image_path.file_path.string());
    logger->debug("Background texture is located at {}", image_cpath);

    layer.z_index = z_index;
    layer.surface.reset(IMG_Load(image_cpath.c_str()), ::SDLDeleter());

    if (!layer.surface) {
      logger->error("Sprite texture failed to load from {}: {}", image_cpath);
      return nullptr;
    }

    layer.bbox.x = 0;
    layer.bbox.y = 0;
    layer.bbox.w = layer.surface->w;
    layer.bbox.h = layer.surface->h;

    bg->layers.push_back(layer);
  }

  std::reverse(bg->layers.begin(), bg->layers.end());

  return bg;
}

void Background::render(Renderer* renderer, const SDL_Rect& clip)
{
  for (auto& layer : layers) {
    if (!layer.texture) {
      layer.texture.reset(renderer->create_texture(layer.surface), ::SDLDeleter());
    }

    int32_t cx = clip.x + clip.w / 2.0;
    int32_t cy = clip.y + clip.h / 2.0;

    auto transformed_dst = layer.bbox;
    transformed_dst.x -= cx * (1.0 - (layer.z_index / 100.0));
    transformed_dst.y -= cy * (layer.z_index / 100.0);

    SDL_RenderCopyEx(renderer->sdl.renderer, layer.texture.get(), &layer.bbox, &transformed_dst, 0, nullptr, SDL_FLIP_NONE);
  }
}

} // raptr