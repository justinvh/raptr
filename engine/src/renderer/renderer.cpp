#include <memory>
#include <algorithm>
#include <numeric>

#include <raptr/game/entity.hpp>
#include <raptr/config.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/parallax.hpp>
#include <raptr/common/clock.hpp>
#include <raptr/common/logging.hpp>

constexpr int32_t GAME_WIDTH = 480 * 2;
constexpr int32_t GAME_HEIGHT = 270 * 2;

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

void SDLDeleter::operator()(SDL_Texture* p) const
{
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

namespace raptr
{
Renderer::~Renderer()
{
  if (sdl.renderer) {
    SDL_DestroyRenderer(sdl.renderer);
  }
  if (sdl.window) {
    SDL_DestroyWindow(sdl.window);
  }
}

void Renderer::scale(const float ratio)
{
  const int32_t w = GAME_WIDTH * ratio;
  const int32_t h = GAME_HEIGHT * ratio;
  desired_ratio = ratio;
  desired_size.w = w;
  desired_size.h = h;
  ratio_per_second = (desired_ratio - current_ratio) / 1.0;
}

void Renderer::scale_to_height(const int32_t height)
{
  const auto ratio = float(height) / GAME_HEIGHT;
  const int32_t w = GAME_WIDTH * ratio;
  const int32_t h = GAME_HEIGHT * ratio;
  desired_ratio = ratio;
  desired_size.w = w;
  desired_size.h = h;
  ratio_per_second = (desired_ratio - current_ratio) / 1.0;
}

void Renderer::scale_to_width(const int32_t width)
{
  const auto ratio = float(width) / GAME_WIDTH;
  const int32_t w = GAME_WIDTH * ratio;
  const int32_t h = GAME_HEIGHT * ratio;
  desired_ratio = ratio;
  desired_size.w = w;
  desired_size.h = h;
  ratio_per_second = (desired_ratio - current_ratio) / 1.0;
}

bool Renderer::init(std::shared_ptr<Config>& config)
{
  this->config = config;

  fps = 120;
  last_render_time_us = clock::ticks();

  if (is_headless) {
    return true;
  }

  zero_offset.x = 0;
  zero_offset.y = -GAME_HEIGHT;

  // Initialize SDL with some basics
  sdl.window = SDL_CreateWindow("RAPTR", 10, 10, 960, 540, 0);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED);
  camera.pos.x = 0;
  camera.pos.y = 0;

  logical_size.x = 0;
  logical_size.y = 0;
  logical_size.w = GAME_WIDTH;
  logical_size.h = GAME_HEIGHT;
  desired_size = logical_size;
  current_ratio = 1.0;
  desired_ratio = 1.0;
  frame_counter_time_start = clock::ticks();
  frame_counter = 0;
  frame_fps = 0;
  total_frames_rendered = 0;
  SDL_RenderSetLogicalSize(sdl.renderer, logical_size.w, logical_size.h);
  SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, 255);
  SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_BLEND);
  return true;
}

void Renderer::run_frame(bool force_render)
{
  if (is_headless) {
    return;
  }

  const auto ms = (clock::ticks() - last_render_time_us) / 1e3;
  last_render_time_us = clock::ticks();
  if (std::fabs(current_ratio - desired_ratio) > 1e-5) {
    const auto delta_ratio_ms = ratio_per_second / 1000.0 * ms;
    current_ratio += delta_ratio_ms;
    if (delta_ratio_ms > 0 && current_ratio > desired_ratio ||
      delta_ratio_ms < 0 && current_ratio < desired_ratio) {
      current_ratio = desired_ratio;
      logical_size = desired_size;
    } else {
      const int32_t w = GAME_WIDTH * current_ratio;
      const int32_t h = GAME_HEIGHT * current_ratio;
      logical_size.w = w;
      logical_size.h = h;
    }
    SDL_RenderSetLogicalSize(sdl.renderer, logical_size.w, logical_size.h);
  }

  SDL_RenderClear(sdl.renderer);
  const auto num_entities = entities_followed.size();
  size_t index = 0;

  struct ClipCamera
  {
    SDL_Rect clip, viewport;
    int32_t left_offset;
    std::vector<std::shared_ptr<Entity>> contains;
  };

  // Determine the cameras
  std::vector<ClipCamera> hclippings;

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

  int32_t min_rect_w, min_rect_h;
  if (num_entities == 0) {
    min_rect_w = logical_size.w;
    min_rect_h = logical_size.h;
  } else {
    min_rect_w = static_cast<int32_t>((logical_size.w / num_entities));
  }

  int32_t min_rect_hw = min_rect_w / 2;

  int32_t current_entity_index = 0;
  int32_t num_clips = 1;
  int32_t last_left = 0;
  int32_t last_center_offset = 0;
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
      clip_cam.contains.push_back(entity);
    }

    int32_t t_right = right;
    while (++current_entity_index < num_entities) {
      const auto& entity = entities_followed[current_entity_index];
      auto& pos = entity->position();
      auto bbox = entity->bbox()[0];
      int32_t x = static_cast<int32_t>(pos.x + bbox.w / 2.0);
      t_right += min_rect_hw;
      if (x <= t_right) {
        clip_cam.contains.push_back(entity);
        y_pos.push_back(static_cast<int32_t>(pos.y + bbox.h));
        right += min_rect_w;
        t_right = right;
      } else {
        break;
      }
    }

    int32_t min_player = *std::max_element(y_pos.begin(), y_pos.end());
    clip_cam.clip.x = left;
    clip_cam.clip.y = 0;
    clip_cam.clip.y = y_pos[0] - GAME_HEIGHT + 64;
    clip_cam.clip.w = right - left;
    clip_cam.clip.h = logical_size.h;
    clip_cam.left_offset = last_center_offset;

    clip_cam.viewport.x = last_left;
    clip_cam.viewport.y = 0;
    clip_cam.viewport.w = clip_cam.clip.w - 1;
    clip_cam.viewport.h = clip_cam.clip.h;

    last_left += clip_cam.clip.w;
    last_center_offset += clip_cam.viewport.w;

    hclippings.push_back(clip_cam);
    ++num_clips;
  }

  std::vector<ClipCamera> clippings = hclippings;

  /*
  // So, the idea here is to iterate through our current horizontal cuts and find what entities are in it and then
  // split the horizontal window in a similar way
  for (auto& hclip_cam : hclippings) {
    int32_t top, bottom;

    // Now, create vertical clips
    std::sort(hclip_cam.contains.begin(),
      hclip_cam.contains.end(),
      [](const auto& lhs, const auto& rhs)
    {
      auto& p1 = lhs->position();
      auto& p2 = rhs->position();
      return p1.y > p2.y;
    });

    int32_t last_top = GAME_HEIGHT;
    ClipCamera clip_cam = hclip_cam;
    clip_cam.contains.clear();
    current_entity_index = 0;
    min_rect_h = static_cast<int32_t>((GAME_HEIGHT / hclip_cam.contains.size()));
    int32_t min_rect_hh = min_rect_h / 2;

    while (current_entity_index < hclip_cam.contains.size()) {
      // Setup the initial camera where we are going to try and merge players together
      {
        const auto& entity = hclip_cam.contains[current_entity_index];
        auto& pos = entity->position();
        auto bbox = entity->bbox()[0];
        int32_t y = pos.y + bbox.h / 2.0;
        top = y - min_rect_hh;
        bottom = y + min_rect_hh;
        clip_cam.contains.push_back(entity);
      }

      int32_t t_top = top;
      while (++current_entity_index < hclip_cam.contains.size()) {
        const auto& entity = hclip_cam.contains[current_entity_index];
        auto& pos = entity->position();
        auto bbox = entity->bbox()[0];
        int32_t y = static_cast<int32_t>(pos.y + bbox.h / 2.0);
        t_top -= min_rect_hh;
        if (y >= t_top) {
          clip_cam.contains.push_back(entity);
          top -= min_rect_h;
          t_top = top;
        } else {
          break;
        }
      }

      clip_cam.clip.y = top;
      clip_cam.clip.h = (bottom - top);

      clip_cam.viewport.y = last_top - clip_cam.clip.h;
      clip_cam.viewport.h = clip_cam.clip.h - 1;

      last_top -= clip_cam.clip.h;

      clippings.push_back(clip_cam);
      ++num_clips;
    }
  }
  */

  if (clippings.empty()) {
    ClipCamera clip_cam;
    clip_cam.clip.x = 0;
    clip_cam.left_offset = 0;
    clip_cam.clip.y = 0;
    clip_cam.clip.w = logical_size.w;
    clip_cam.clip.h = logical_size.h;

    clip_cam.viewport.x = 0;
    clip_cam.viewport.y = 0;
    clip_cam.viewport.w = clip_cam.clip.w - 1;
    clip_cam.viewport.h = GAME_HEIGHT;
    clippings.push_back(clip_cam);
  }

  for (auto& e : observing) {
    e->render(this);
  }

  for (const auto& clip_cam : clippings) {
    SDL_RenderSetViewport(sdl.renderer, &clip_cam.viewport);

    SDL_Rect bg_clip = clip_cam.clip;
    bg_clip.x -= clip_cam.left_offset;
    for (auto& background : backgrounds) {
      background->render(this, bg_clip, clip_cam.left_offset);
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
      foreground->render(this, bg_clip, clip_cam.left_offset);
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
  will_render_middle.clear();
  will_render_foreground.clear();

  ++frame_counter;
  ++total_frames_rendered;

  if (clock::ticks() - frame_counter_time_start >= 5e6) {
    logger->debug("FPS = {} ({} frames in 5s)", frame_counter / 5.0, frame_counter);
    frame_counter_time_start = clock::ticks();
    frame_fps = frame_counter / 5.0;
    frame_counter = 0;
  }
}

bool Renderer::toggle_fullscreen() const
{
  if (is_headless) {
    return false;
  }

  if (SDL_GetWindowFlags(sdl.window) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
    SDL_SetWindowFullscreen(sdl.window, 0);
    return false;
  }
  SDL_SetWindowFullscreen(sdl.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  return true;
}

void Renderer::camera_follow(std::vector<std::shared_ptr<Entity>> entities)
{
  entities_followed = entities;
}

void Renderer::camera_follow(std::shared_ptr<Entity> entity)
{
  entities_followed.push_back(entity);
}

SDL_Texture* Renderer::create_texture(std::shared_ptr<SDL_Surface>& surface) const
{
  if (is_headless) {
    return nullptr;
  }

  return SDL_CreateTextureFromSurface(sdl.renderer, surface.get());
}

void Renderer::add_texture(std::shared_ptr<SDL_Texture>& texture,
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
