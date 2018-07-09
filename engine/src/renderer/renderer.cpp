#include <memory>
#include <algorithm>
#include <numeric>

#include <raptr/game/entity.hpp>
#include <raptr/config.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/parallax.hpp>
#include <raptr/common/clock.hpp>

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

  fps = 60;
  last_render_time_us = clock::ticks();

  if (is_headless) {
    return true;
  }

  // Initialize SDL with some basics
  sdl.window = SDL_CreateWindow("RAPTR", 10, 10, 960, 540, 0);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED);
  camera.pos.x = 0;
  camera.pos.y = 0;
  SDL_RenderSetLogicalSize(sdl.renderer, GAME_WIDTH, GAME_HEIGHT);
  SDL_SetRenderDrawColor(sdl.renderer, 255, 255, 255, 255);
  SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_BLEND);
  return true;
}

void Renderer::run_frame(bool force_render)
{
  if (is_headless) {
    return;
  }

  int64_t ms = static_cast<int64_t>((clock::ticks() - last_render_time_us) / 1e3);
  if (!force_render && ms < 16) {
    return;
  }

  SDL_RenderClear(sdl.renderer);
  size_t num_entities = entities_followed.size();
  size_t index = 0;

  struct ClipCamera {
    SDL_Rect clip, viewport;
  };

  // Determine the cameras
  std::vector<ClipCamera> clippings;

  // Sort the entities being followed left to right to figure
  // out how we're going to clip them
  std::sort(entities_followed.begin(),
            entities_followed.end(),
            [](const auto& lhs, const auto& rhs)
            {
              auto& p1 = lhs->position();
              auto& p2 = rhs->position();
              return p1.x < p2.x;
            });

  int32_t min_rect_w;
  if (num_entities == 0) {
    min_rect_w = GAME_WIDTH;
  } else {
    min_rect_w = static_cast<int32_t>((GAME_WIDTH / num_entities));
  }

  int32_t min_rect_hw = min_rect_w / 2;

  int32_t current_entity_index = 0;
  int32_t num_clips = 1;
  int32_t last_left = 0;
  std::vector<int32_t> y_pos;
  while (current_entity_index < num_entities) {
    int32_t left, right, top, bottom;
    ClipCamera clip_cam;

    // Setup the initial camera where we are going to try and merge players together
    {
      const auto& entity = entities_followed[current_entity_index];
      auto& pos = entity->position();
      auto bbox = entity->bbox()[0];

      int32_t x = static_cast<int32_t>((pos.x + bbox.w / 2.0));
      left = x - min_rect_hw;
      right = x + min_rect_hw;
      y_pos.push_back(static_cast<int32_t>(pos.y + bbox.h));
      top = 0;
      bottom = GAME_HEIGHT;
    }

    int32_t t_right = right;
    while (++current_entity_index < num_entities) {
      const auto& entity = entities_followed[current_entity_index];
      auto& pos = entity->position();
      auto bbox = entity->bbox()[0];
      int32_t x = static_cast<int32_t>(pos.x + bbox.w / 2.0);
      t_right += min_rect_hw;
      if (x <= t_right) {
        y_pos.push_back(static_cast<int32_t>(pos.y + bbox.h));
        right += min_rect_w;
        t_right = right;
      } else {
        break;
      }
    }

    int32_t min_player = *std::max_element(y_pos.begin(), y_pos.end());
    clip_cam.clip.x = left;
    clip_cam.clip.y = (y_pos[0] + 32) - GAME_HEIGHT; // GAME_HEIGHT - std::accumulate(y_pos.begin(), y_pos.end(), 0) / y_pos.size();
    clip_cam.clip.w = (right - left);
    clip_cam.clip.h = GAME_HEIGHT;

    clip_cam.viewport.x = last_left;
    clip_cam.viewport.y = 0;
    clip_cam.viewport.w = clip_cam.clip.w - 1;
    clip_cam.viewport.h = clip_cam.clip.h;

    last_left += clip_cam.clip.w;

    clippings.push_back(clip_cam);
    ++num_clips;
  }

  if (clippings.empty()) {
    ClipCamera clip_cam;
    clip_cam.clip.x = 0;
    clip_cam.clip.y = 0; 
    clip_cam.clip.w = GAME_WIDTH;
    clip_cam.clip.h = GAME_HEIGHT;

    clip_cam.viewport.x = 0;
    clip_cam.viewport.y = 0;
    clip_cam.viewport.w = clip_cam.clip.w - 1;
    clip_cam.viewport.h = clip_cam.clip.h;
    clippings.push_back(clip_cam);
  }

  for (auto& e : observing) {
    e->render(this);
  }

  for (const auto& clip_cam : clippings) {
    SDL_RenderSetViewport(sdl.renderer, &clip_cam.viewport);

    for (auto& background : backgrounds) {
      background->render(this, clip_cam.clip);
    }

    for (auto w : will_render_middle) {
      auto transformed_dst = w.dst;

      if (!w.absolute_positioning) {
        transformed_dst.x -= clip_cam.clip.x;
        transformed_dst.y -= clip_cam.clip.y;
      }

      SDL_RenderCopyEx(sdl.renderer, w.texture.get(), &w.src, &transformed_dst,
        w.angle, nullptr, static_cast<SDL_RendererFlip>(w.flip_mask()));
    }

    for (auto& foreground : foregrounds) {
      foreground->render(this, clip_cam.clip);
    }

    for (auto w : will_render_foreground) {
      auto transformed_dst = w.dst;

      if (!w.absolute_positioning) {
        transformed_dst.x -= clip_cam.clip.x;
        transformed_dst.y -= clip_cam.clip.y;
      }

      SDL_RenderCopyEx(sdl.renderer, w.texture.get(), &w.src, &transformed_dst,
        w.angle, nullptr, static_cast<SDL_RendererFlip>(w.flip_mask()));
    }
    ++index;
  }

  SDL_RenderPresent(sdl.renderer);
  last_render_time_us = clock::ticks();
  will_render_middle.clear();
  will_render_foreground.clear();
  ++frame_count;
}

bool Renderer::toggle_fullscreen()
{
  if (is_headless) {
    return false;
  }

  if (SDL_GetWindowFlags(sdl.window) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
    SDL_SetWindowFullscreen(sdl.window, 0);
    return false;
  } else {
    SDL_SetWindowFullscreen(sdl.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    return true;
  }
}

void Renderer::camera_follow(std::vector<std::shared_ptr<Entity>> entities)
{
  entities_followed = entities;
}

SDL_Texture* Renderer::create_texture(std::shared_ptr<SDL_Surface>& surface) const
{
  if (is_headless) {
    return nullptr;
  }

  return SDL_CreateTextureFromSurface(sdl.renderer, surface.get());
}

void Renderer::add(std::shared_ptr<SDL_Texture>& texture,
                   SDL_Rect src, SDL_Rect dst,
                   float angle, bool flip_x, bool flip_y,
                   bool absolute_positioning,
                   bool render_in_foreground)
{
  if (is_headless) {
    return;
  }

  Renderable renderable = {texture, src, dst, angle, flip_x, flip_y, absolute_positioning};
  if (render_in_foreground) {
    will_render_foreground.push_back(renderable);
  } else {
    will_render_middle.push_back(renderable);
  }
}

void Renderer::add_background(std::shared_ptr<Parallax>& background)
{
  backgrounds.push_back(background);
}

void Renderer::add_foreground(std::shared_ptr<Parallax>& foreground)
{
  foregrounds.push_back(foreground);
}

} // namespace raptr

