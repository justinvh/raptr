#include <fstream>
#include <iostream>
#include <map>
#include <picojson.h>
#include <SDL_image.h>

#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/sprite.hpp>

namespace raptr {

std::map<std::string, std::shared_ptr<SDL_Surface>> SURFACE_CACHE;
std::map<std::shared_ptr<SDL_Surface>, std::shared_ptr<SDL_Texture>> TEXTURE_CACHE;

int32_t p_int(const picojson::value& v, const std::string& name)
{
  return static_cast<int32_t>(v.get(name).get<double>());
}

std::string p_string(const picojson::value& v, const std::string& name)
{
  return v.get(name).get<std::string>();
}

std::shared_ptr<Sprite> Sprite::from_json(const std::string& path)
{
  std::shared_ptr<Sprite> sprite(new Sprite);
  std::ifstream input(path);

  picojson::value doc;
  input >> doc;

  auto frames = doc.get("frames").get<picojson::array>();
  auto meta = doc.get("meta");
  auto tags = meta.get("frameTags").get<picojson::array>();
  auto size = meta.get("size");

  sprite->width = p_int(size, "w");
  sprite->height = p_int(size, "h");

  auto image_path = p_string(meta, "image");

  auto exists = SURFACE_CACHE.find(image_path);
  if (exists != SURFACE_CACHE.end()) {
    sprite->surface = exists->second;
  } else {
    SURFACE_CACHE[image_path].reset(IMG_Load(image_path.c_str()), SDLDeleter());
    sprite->surface = SURFACE_CACHE[image_path];
  }

  for (auto tag : tags) {
    auto tag_name = p_string(tag, "name");
    auto from = p_int(tag, "from");
    auto to = p_int(tag, "to");
    auto direction = p_string(tag, "direction");

    Animation animation;
    animation.name = tag_name;
    animation.frame = 0;
    animation.from = 0;
    animation.to = to - from;
    animation.ping_backwards = false;
    animation.hold_last_frame = false;

    if (direction == "forward") {
      animation.direction = AnimationDirection::forward;
    } else if (direction == "backward") {
      animation.direction = AnimationDirection::backward;
    } else if (direction == "pingpong") {
      animation.direction = AnimationDirection::ping_pong;
    } else {
      std::cerr << direction << " is not a recognized animation direction\n";
      throw std::runtime_error("Unknown animation direction");
    }

    for (int32_t frame_num = from; frame_num <= to; ++frame_num) {
      AnimationFrame anim_frame;

      auto frame = frames.at(frame_num);
      auto frame_name = p_string(frame, "filename");
      auto frame_size = frame.get("frame");
      anim_frame.x = p_int(frame_size, "x");
      anim_frame.y = p_int(frame_size, "y");
      anim_frame.w = p_int(frame_size, "w");
      anim_frame.h = p_int(frame_size, "h");
      anim_frame.duration = p_int(frame, "duration");

      animation.speed = 1.0f;
      animation.frames.push_back(anim_frame);
    }

    sprite->animations[tag_name] = animation;
  }

  sprite->current_animation = &sprite->animations["Idle"];
  sprite->last_frame_tick = SDL_GetTicks();
  sprite->scale = 1.0;
  sprite->flip_x = false;
  sprite->flip_y = false;
  sprite->angle = 0.0;

  return sprite;
}

void Sprite::render(std::shared_ptr<Renderer> renderer)
{
  if (!texture) {
    auto exists = TEXTURE_CACHE.find(surface);
    if (exists == TEXTURE_CACHE.end()) {
      TEXTURE_CACHE[surface].reset(renderer->create_texture(surface), SDLDeleter());
      texture = TEXTURE_CACHE[surface];
    } else {
      texture = exists->second;
    }
  }

  auto frame = current_animation->frames[current_animation->frame];
  if (current_animation->next(last_frame_tick)) {
    last_frame_tick = SDL_GetTicks();
  }

  SDL_Rect src, dst;

  src.w = frame.w;
  src.h = frame.h;
  src.x = frame.x;
  src.y = frame.y;

  dst.w = static_cast<int32_t>(frame.w * scale);
  dst.h = static_cast<int32_t>(frame.h * scale);
  dst.x = static_cast<int32_t>(x);
  dst.y = static_cast<int32_t>(y);

  renderer->add(texture, src, dst, angle, flip_x, flip_y);
}


void Sprite::set_animation(const std::string& name, bool hold_last_frame)
{
  if (current_animation->name == name) {
    return;
  }

  current_animation = &animations[name];
  current_animation->frame = 0;
  current_animation->hold_last_frame = hold_last_frame;
}

}