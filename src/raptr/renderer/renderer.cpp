#include <memory>

#include <raptr/game/entity.hpp>
#include <raptr/config.hpp>
#include <raptr/renderer/renderer.hpp>

constexpr int32_t GAME_WIDTH = 480;
constexpr int32_t GAME_HEIGHT = 270;

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
  camera.pos.x = 0;
  camera.pos.y = 0;
  SDL_RenderSetLogicalSize(sdl.renderer, GAME_WIDTH, GAME_HEIGHT);
  return true;
}

void Renderer::run_frame()
{
  SDL_RenderClear(sdl.renderer);

  if (entity_followed) {
    auto& pos = entity_followed->position();
    int64_t left = pos.x - GAME_WIDTH / 2;
    int64_t right = pos.x + GAME_WIDTH / 2;
    int64_t top = pos.y - GAME_HEIGHT / 2;
    int64_t bottom = pos.y + GAME_HEIGHT / 2;

    if (left >= camera.left && right <= camera.right)
    {
      camera.pos.x = left;
    }

    if (bottom <= camera.bottom && top >= camera.top) {
      camera.pos.y = top;
    }
  }

  for (auto w : will_render) {
    auto transformed_dst = w.dst;

    if (!w.absolute_positioning) {
      transformed_dst.x -= camera.pos.x;
      transformed_dst.y -= camera.pos.y;
    }

    SDL_RenderCopyEx(sdl.renderer, w.texture.get(), &w.src, &transformed_dst,
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

void Renderer::camera_follow(std::shared_ptr<Entity>& entity)
{
  entity_followed = entity;
}

SDL_Texture* Renderer::create_texture(std::shared_ptr<SDL_Surface>& surface) const
{
  return SDL_CreateTextureFromSurface(sdl.renderer, surface.get());
}

void Renderer::add(std::shared_ptr<SDL_Texture>& texture,
                   SDL_Rect src, SDL_Rect dst,
                   float angle, bool flip_x, bool flip_y,
                   bool absolute_positioning)
{
  Renderable renderable = {texture, src, dst, angle, flip_x, flip_y, absolute_positioning};
  will_render.push_back(renderable);
}

} // namespace raptr

