#include <SDL2/SDL.h>

#include "config.hpp"
#include "renderer.hpp"

namespace raptr {

bool Renderer::init(std::shared_ptr<Config> config_)
{
  config = config_;

  // Initialize SDL with some basics
  sdl.window = SDL_CreateWindow("RAPTR", 100, 100, 1000, 1000, 0);
  sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED);
  return true;
}

void Renderer::run_frame()
{
  SDL_RenderClear(sdl.renderer);

  for (auto w : will_render) {
    SDL_RenderCopyEx(sdl.renderer, w.texture.get(), &w.src, &w.dst, 
                     w.angle, nullptr, static_cast<SDL_RendererFlip>(w.flip_mask()));
  }

  will_render.clear();
  SDL_RenderPresent(sdl.renderer);
  ++frame_count;
}

SDL_Texture* Renderer::create_texture(std::shared_ptr<SDL_Surface> surface)
{
  return SDL_CreateTextureFromSurface(sdl.renderer, surface.get());
}

void Renderer::add(std::shared_ptr<SDL_Texture> texture, 
                   SDL_Rect src, SDL_Rect dst,
                   float angle, bool flip_x, bool flip_y)
{
  Renderable renderable = {texture, src, dst, angle, flip_x, flip_y};
  will_render.push_back(renderable);
}

}

