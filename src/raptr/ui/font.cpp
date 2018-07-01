#include <map>
#include <raptr/common/logging.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/ui/font.hpp>

#pragma warning(disable : 4996)
#include <toml/toml.h>
#pragma warning(default : 4996)

macro_enable_logger();

namespace raptr {

namespace {

bool FONTS_REGISTERED = false;
typedef std::pair<std::string, int32_t> FontAndSize;
typedef std::map<FontAndSize, std::shared_ptr<TTF_Font>> FontToTTF;
FontToTTF FONT_REGISTRY;

}

bool load_registry(const FileInfo& game_root)
{
  if (FONTS_REGISTERED) {
    return true;
  }

  if (TTF_Init() < 0) {
    logger->error("TTF failed to initialize: {}", SDL_GetError());
    return false;
  }

  auto toml_path = game_root.from_root("fonts/fonts.toml");
  auto toml_relative = toml_path.file_relative;
  auto ifs = toml_path.open();
  if (!ifs) {
    return false;
  }

  toml::ParseResult pr = toml::parse(*ifs);

  if (!pr.valid()) {
    logger->error("Failed to parse {} with reason {}", toml_relative, pr.errorReason);
    return nullptr;
  }

  const toml::Value& v = pr.value;

  const toml::Array& registry_values = v.find("font")->as<toml::Array>();
  for (const toml::Value& v : registry_values) {
    std::string name = v.get<std::string>("name");
    std::string rel_path = v.get<std::string>("path");
    FileInfo full_path = game_root.from_root(fs::path("fonts") / rel_path);
    std::string full_path_str = full_path.file_path.string();
    logger->info("Loading 8-24px {} from fonts/{}", name, rel_path);

    for (int i = 8; i <= 24; ++i) {
      // Load the font
      auto ttf = TTF_OpenFont(full_path_str.c_str(), i);
      if (!ttf) {
        logger->error("TTF failed to load font {}: {}", full_path, SDL_GetError());
        return false;
      }

      // Handover the TTF to font
      std::shared_ptr<TTF_Font> font;
      font.reset(ttf);

      // Create the FONT_REGISTRY
      auto pair = std::make_pair(name, i);
      FONT_REGISTRY[pair] = font;
    }
  }

  FONTS_REGISTERED = true;
  return true;
}

bool Text::allocate(std::shared_ptr<Renderer>& renderer)
{
  if (texture) {
    return false;
  }

  texture.reset(renderer->create_texture(surface), SDLDeleter());
  if (!texture) {
    logger->error("Failed to allocate texture");
    return false;
  }

  SDL_QueryTexture(texture.get(), NULL, NULL, &bbox.w, &bbox.h);
  bbox.x = 0;
  bbox.y = 0;

  return true;
}

std::shared_ptr<Text> Text::create(const FileInfo& game_root,
                                   const std::string& font,
                                   const std::string& text,
                                   int32_t size,
                                   const SDL_Color& fg)
{
  if (!load_registry(game_root)) {
    logger->error("Registry failed to initialize");
    return nullptr;
  }

  FontAndSize font_size(font, size);
  auto ttf = FONT_REGISTRY.find(font_size);
  if (ttf == FONT_REGISTRY.end()) {
    logger->error("Failed to load {} with font size {}", font, size);
    return nullptr;
  }

  std::shared_ptr<Text> text_obj(new Text);
  auto renderable = TTF_RenderText_Solid(
    ttf->second.get(),
    text.c_str(),
    fg
  );

  text_obj->surface.reset(renderable);
  if (!text_obj->surface) {
    logger->error("Failed to render surface {} with font size {}", font, size);
    return false;
  }

  return text_obj;
}

}
