#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <memory>

namespace raptr {

class Config;

struct Renderable {
  std::shared_ptr<SDL_Texture> texture;
  SDL_Rect src, dst;
  float angle;
  bool flip_x;
  bool flip_y;

  int flip_mask()
  {
    int out;
    if (flip_x && flip_y) {
      out = SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL;
    } else if (flip_x) {
      out = SDL_FLIP_HORIZONTAL;
    } else if (flip_y) {
      out = SDL_FLIP_VERTICAL;
    } else {
      out = SDL_FLIP_NONE;
    }
    return out;
  }
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

  bool toggle_fullscreen();

  bool init(std::shared_ptr<Config> config_);
  void run_frame();
  SDL_Texture* create_texture(std::shared_ptr<SDL_Surface> surface);
  void add(std::shared_ptr<SDL_Texture> texture, 
           SDL_Rect src, SDL_Rect dst,
           float angle,  bool flip_x, bool flip_y);

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
