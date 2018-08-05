#include <memory>
#include <algorithm>
#include <numeric>

//#include <glad/glad.h>

#include <raptr/game/entity.hpp>
#include <raptr/config.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/parallax.hpp>
#include <raptr/common/clock.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/ui/font.hpp>

constexpr int32_t GAME_WIDTH = 720;
constexpr int32_t GAME_HEIGHT = 405;

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
  const auto w = static_cast<int32_t>(GAME_WIDTH * ratio);
  const auto h = static_cast<int32_t>(GAME_HEIGHT * ratio);
  desired_ratio = ratio;
  desired_size.w = w;
  desired_size.h = h;
  ratio_per_second = (desired_ratio - current_ratio) / 1.0;
}

void Renderer::scale_to_height(const int32_t height)
{
  const auto ratio = float(height) / GAME_HEIGHT;
  const auto w = static_cast<int32_t>(GAME_WIDTH * ratio);
  const auto h = static_cast<int32_t>(GAME_HEIGHT * ratio);
  desired_ratio = ratio;
  desired_size.w = w;
  desired_size.h = h;
  ratio_per_second = (desired_ratio - current_ratio) / 1.0;
}

void Renderer::scale_to_width(const int32_t width)
{
  const auto ratio = float(width) / GAME_WIDTH;
  const auto w = static_cast<int32_t>(GAME_WIDTH * ratio);
  const auto h = static_cast<int32_t>(GAME_HEIGHT * ratio);
  desired_ratio = ratio;
  desired_size.w = w;
  desired_size.h = h;
  ratio_per_second = (desired_ratio - current_ratio) / 1.0;
}

bool Renderer::init(std::shared_ptr<Config>& config)
{
  this->config = config;

  fps = 60;
  show_fps = false;
  last_render_time_us = clock::ticks();

  if (is_headless) {
    return true;
  }

  zero_offset.x = 0;
  zero_offset.y = -GAME_HEIGHT;

  /*
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  */

  // Initialize SDL with some basics
  sdl.window = SDL_CreateWindow("RAPTR", 10, 10, 960, 540, SDL_WINDOW_SHOWN);

  //sdl.gl = SDL_GL_CreateContext(sdl.window);

  /*
  if (!sdl.gl) {
    logger->error("OpenGL context could not be created: {}", SDL_GetError());
    return false;
  }
  */

  /*
  if (!gladLoadGLLoader(static_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
    logger->error("Failed to initialize GLAD!");
    return false;
  }
  */

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
  sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_ACCELERATED);
  camera_basic.pos.x = 0;
  camera_basic.pos.y = 0;
  camera_basic.min_x = -10000;
  camera_basic.max_x = 10000;

  camera = Camera({GAME_WIDTH / 2, GAME_HEIGHT / 2}, GAME_WIDTH, GAME_HEIGHT);

  logical_size.x = 0;
  logical_size.y = 0;
  logical_size.w = GAME_WIDTH;
  logical_size.h = GAME_HEIGHT;
  desired_size = logical_size;
  window_size = logical_size;
  render_err_us = 0;
  current_ratio = 1.0;
  desired_ratio = 1.0;
  frame_counter_time_start = clock::ticks();
  frame_counter = 0;
  frame_fps = 0;
  total_frames_rendered = 0;
  SDL_RenderSetLogicalSize(sdl.renderer, logical_size.w, logical_size.h);
  SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, 255);
  SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderSetViewport(sdl.renderer, &logical_size);
  return true;
}

void Renderer::run_frame(bool force_render)
{
  std::scoped_lock<std::mutex> lck(mutex);

  if (is_headless) {
    return;
  }

  SDL_RenderClear(sdl.renderer);

  const auto render_delta_us = (clock::ticks() - last_render_time_us);
  const auto render_delta_ms = render_delta_us * 1e3;

  /*
  if (render_delta_us + render_err_us < 1e6 / fps) {
    return;
  }
  */

  render_err_us = static_cast<int64_t>((render_delta_us + render_err_us) - 1e6 / fps);
  last_render_time_us = clock::ticks();

  camera.think(this, render_delta_us);

  for (auto& e : observing) {
    e->render(this);
  }

  int32_t num_objects_rendered = 0;

  for (const auto& clip_cam : camera.clips) {

    camera.render(this, clip_cam);

    auto bg_clip = clip_cam.clip;
    bg_clip.x -= clip_cam.left_offset;
    for (auto& background : backgrounds) {
      background->render(this, bg_clip, clip_cam.left_offset);
      ++num_objects_rendered;
    }

    for (const auto& w : will_render_middle) {
      if (w->render(this, clip_cam)) {
        ++num_objects_rendered;
      }
    }

    for (auto& foreground : foregrounds) {
      foreground->render(this, bg_clip, clip_cam.left_offset);
      ++num_objects_rendered;
    }

    for (const auto& w : will_render_foreground) {
      if (w->render(this, clip_cam)) {
        ++num_objects_rendered;
      }
    }
  }

  SDL_RenderPresent(sdl.renderer);
  will_render_middle.clear();
  will_render_foreground.clear();

  ++frame_counter;
  ++total_frames_rendered;

  if (show_fps) {
    if (clock::ticks() - frame_counter_time_start >= 1e6) {
      std::stringstream ss;
      const auto secs = (clock::ticks() - frame_counter_time_start) / 1e6;
      const auto current_fps = static_cast<int32_t>(frame_counter / secs);
      ss << current_fps << " FPS";
      fps_text = this->add_text({5, 0}, ss.str(), 20);

      ss = std::stringstream();
      ss << num_objects_rendered << " objects rendered";
      num_obj_rendered_text = this->add_text({5, 20}, ss.str(), 20);

      frame_counter_time_start = clock::ticks();
      frame_fps = static_cast<float>(frame_counter / secs);
      frame_counter = 1;
    }

    if (fps_text) {
      fps_text->render(this, {5, 0});
    }

    if (num_obj_rendered_text) {
      num_obj_rendered_text->render(this, {5, 20});
    }
  }
}

bool Renderer::toggle_fullscreen() 
{
  //std::scoped_lock<add_object_mutex> lck();
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
  camera.tracking = entities;
}

void Renderer::camera_follow(std::shared_ptr<Entity> entity)
{
  camera.tracking.push_back(entity);
}

SDL_Texture* Renderer::create_texture(std::shared_ptr<SDL_Surface>& surface) 
{
  if (is_headless) {
    return nullptr;
  }

  return SDL_CreateTextureFromSurface(sdl.renderer, surface.get());
}

void Renderer::add_texture(std::shared_ptr<SDL_Texture> texture,
                   SDL_Rect src, SDL_Rect dst,
                   float angle, bool flip_x, bool flip_y,
                   bool absolute_positioning,
                   bool render_in_foreground)
{
  if (is_headless) {
    return;
  }

  //auto renderable = std::make_shared<RenderableTexture>();
  auto renderable = std::shared_ptr<RenderableTexture>(new RenderableTexture);
  renderable->texture = std::move(texture);
  renderable->src = src;
  renderable->dst = dst;
  renderable->angle = angle;
  renderable->flip_x = flip_x;
  renderable->flip_y = flip_y;
  renderable->absolute_positioning = absolute_positioning;

  if (render_in_foreground) {
    will_render_foreground.push_back(std::move(renderable));
  } else {
    will_render_middle.push_back(std::move(renderable));
  }
}

void Renderer::add_rect(Rect rect, SDL_Color color,
                        bool absolute_positioning,
                        bool render_in_foreground)
{
  const SDL_Rect s_rect = {rect.x, rect.y, rect.w, rect.h};
  return this->add_rect(s_rect, color, absolute_positioning, render_in_foreground);
}

void Renderer::add_rect(SDL_Rect rect, SDL_Color color,
                        bool absolute_positioning,
                        bool render_in_foreground)
{
  auto renderable = std::make_shared<RenderableRect>();
  renderable->rect = rect;
  renderable->color = color;
  renderable->absolute_positioning = absolute_positioning;

  if (render_in_foreground) {
    will_render_foreground.push_back(renderable);
  } else {
    will_render_middle.push_back(renderable);
  }
}

std::shared_ptr<Text> Renderer::add_text(const SDL_Point& position, const std::string& text, 
                                         uint32_t size, SDL_Color color)
{
  auto obj = Text::create(game_root, "default", text, size, color);
  obj->render(this, position);
  return obj;
}

void Renderer::add_background(std::shared_ptr<Parallax> background)
{
  backgrounds.push_back(background);
}

void Renderer::add_foreground(std::shared_ptr<Parallax> foreground)
{
  foregrounds.push_back(foreground);
}

bool RenderableTexture::render(Renderer* renderer, const CameraClip& camera)
{
  auto transformed_dst = dst;

  transformed_dst.y = GAME_HEIGHT - (transformed_dst.y + transformed_dst.h);

  if (!absolute_positioning && 
     (transformed_dst.x + 64) < camera.clip.x || (transformed_dst.x - 64) >(camera.clip.x + camera.clip.w)) {
    return false;
  }

  if (!absolute_positioning) {
    transformed_dst.x -= camera.clip.x;
    transformed_dst.y -= camera.clip.y;
  }

  if (flip_x || flip_y || angle) {
    SDL_RenderCopyEx(renderer->sdl.renderer, texture.get(), &src,
      &transformed_dst, angle, nullptr,
      static_cast<SDL_RendererFlip>(flip_mask()));
  } else {
    SDL_RenderCopy(renderer->sdl.renderer, texture.get(), &src, &transformed_dst);
  }

  return true;
}

bool RenderableRect::render(Renderer* renderer, const CameraClip& camera)
{
  auto transformed_dst = rect;

  transformed_dst.y = GAME_HEIGHT - (transformed_dst.y + transformed_dst.h);

  if (!absolute_positioning && transformed_dst.x < camera.clip.x || 
      transformed_dst.x >(camera.clip.x + camera.clip.w)) 
  {
    return false;
  }

  if (!absolute_positioning) {
    transformed_dst.x -= camera.clip.x;
    transformed_dst.y -= camera.clip.y;
  }

  const auto sdl_rend = renderer->sdl.renderer;
  SDL_SetRenderDrawColor(sdl_rend, color.r, color.g, color.b, color.a);
  SDL_RenderDrawRect(sdl_rend, &transformed_dst);
  return true;
}

void Renderer::setup_lua_context(sol::state& state)
{
  state.new_usertype<Renderer>("Renderer",
    "show_fps", &Renderer::show_fps,
    "toggle_fullscreen", &Renderer::toggle_fullscreen
  );
}

} // namespace raptr
