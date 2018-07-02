#pragma once

#include <memory>
#include <string>
#include <SDL_ttf.h>

#include <raptr/common/filesystem.hpp>

namespace raptr {

class Renderer;

class Text {
 public:
  std::shared_ptr<SDL_Surface> surface;
  std::shared_ptr<SDL_Texture> texture;
  SDL_Rect bbox;
  bool allocate(std::shared_ptr<Renderer>& renderer);

 public:
   static std::shared_ptr<Text> create(const FileInfo& game_root,
                                       const std::string& font,
                                       const std::string& text,
                                       int32_t size,
                                       const SDL_Color& fg,
                                       int32_t max_width = 200);
};

}