#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <memory>

namespace raptr {

class Config;

struct Renderable {
  std::shared_ptr<SDL_Texture> texture;
  SDL_Rect src, dst;
};

class Renderer {
public:
  Renderer() = default;
  Renderer(const Renderer&) = default;
  Renderer(Renderer&&) = default;
  ~Renderer()
  {
    if (sdl.renderer) {
      SDL_DestroyRenderer(sdl.renderer);
    }
    if (sdl.window) {
      SDL_DestroyWindow(sdl.window);
    }
  }
  bool init(std::shared_ptr<Config> config_);
  void run_frame();
  SDL_Texture* create_texture(std::shared_ptr<SDL_Surface> surface);
  void add(std::shared_ptr<SDL_Texture> texture, SDL_Rect src, SDL_Rect dst);

  std::shared_ptr<Config> config;

private:
  struct SDLInternal {
    SDL_Window* window;
    SDL_Renderer* renderer;
  } sdl;

  uint64_t frame_count;
  std::vector<Renderable> will_render;
};

}
