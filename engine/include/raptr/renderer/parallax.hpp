/*!
  \file parallax.hpp
  This module is responsible for being the skybox or parallax background
*/
#pragma once

#include <memory>
#include <vector>
#include <SDL.h>
#include <raptr/common/filesystem.hpp>

namespace raptr
{
struct ParallaxLayer
{
  FileInfo texture_path;
  uint32_t z_index;
  SDL_Rect bbox;
  std::shared_ptr<SDL_Surface> surface;
  std::shared_ptr<SDL_Texture> texture;
};

class Renderer;

class Parallax
{
public:
  static std::shared_ptr<Parallax> from_toml(const FileInfo& path);

public:
  /*!
     Render the background relative to a camera viewport
     \param clip - The clip that is currently being rendered
  */
  void render(Renderer* renderer, const SDL_Rect& clip, int32_t rx);

public:
  SDL_Rect dst;
  bool is_foreground;
  std::vector<ParallaxLayer> layers;
};
}
