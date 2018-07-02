#include <memory>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <raptr/config.hpp>
#include <raptr/renderer/renderer.hpp>

void SDLDeleter::operator()(SDL_Texture* p) const
{
  SDL_DestroyTexture(p);
}

void SDLDeleter::operator()(SDL_Surface* p) const
{
  if (p->refcount == 0) {
    SDL_FreeSurface(p);
  }
}

void SDLDeleter::operator()(TTF_Font* p) const
{
  TTF_CloseFont(p);
}

namespace raptr {

Renderer::~Renderer()
{
  if (sdl.renderer) {
    SDL_DestroyRenderer(sdl.renderer);
  }
  if (sdl.window) {
    SDL_DestroyWindow(sdl.window);
  }
}

bool Renderer::init(std::shared_ptr<Config>& config_)
{
  config = config_;

  // Initialize SDL with some basics
  sdl.window = SDL_CreateWindow("RAPTR", 10, 10, 960, 540, 0);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED);
  SDL_RenderSetLogicalSize(sdl.renderer, 480, 270);
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

bool Renderer::toggle_fullscreen()
{
  if (SDL_GetWindowFlags(sdl.window) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
    SDL_SetWindowFullscreen(sdl.window, 0);
    return false;
  } else {
    SDL_SetWindowFullscreen(sdl.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    return true;
  }
}

SDL_Texture* Renderer::create_texture(std::shared_ptr<SDL_Surface>& surface) const
{
  return SDL_CreateTextureFromSurface(sdl.renderer, surface.get());
}

void Renderer::add(std::shared_ptr<SDL_Texture>& texture,
                   SDL_Rect src, SDL_Rect dst,
                   float angle, bool flip_x, bool flip_y)
{
  Renderable renderable = {texture, src, dst, angle, flip_x, flip_y};
  will_render.push_back(renderable);
}

} // namespace raptr

