#include <SDL2/SDL.h>

#include "config.hpp"
#include "renderer.hpp"

namespace raptr {

bool Renderer::init(std::shared_ptr<Config> config_)
{
  config = config_;

  // Initialize SDL with some basics
  sdl.window = SDL_CreateWindow("RAPTR", 100, 100, 640, 480, 0);
  sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED);
  return true;
}

void Renderer::run_frame()
{
  SDL_RenderClear(sdl.renderer);

  for (auto w : will_render) {
    SDL_RenderCopy(sdl.renderer, w.texture.get(), &w.src, &w.dst);
  }

  will_render.clear();
  SDL_RenderPresent(sdl.renderer);
  ++frame_count;
}

SDL_Texture* Renderer::create_texture(std::shared_ptr<SDL_Surface> surface)
{
  return SDL_CreateTextureFromSurface(sdl.renderer, surface.get());
}

void Renderer::add(std::shared_ptr<SDL_Texture> texture, SDL_Rect src, SDL_Rect dst)
{
  Renderable renderable = {texture, src, dst};
  will_render.push_back(renderable);
}

}

