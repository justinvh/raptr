#include <iostream>
#include <string>
#include <memory>
#include <map>

#include <picojson.h>
#include <SDL_image.h>

#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/common/clock.hpp>
#include <raptr/sound/sound.hpp>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr
{
std::map<fs::path, std::shared_ptr<SDL_Surface>> SURFACE_CACHE;
std::map<std::shared_ptr<SDL_Surface>, std::shared_ptr<SDL_Texture>> TEXTURE_CACHE;
std::map<fs::path, std::shared_ptr<Sprite>> SPRITE_CACHE;

int32_t p_int(const picojson::value& v, const std::string& name)
{
  return static_cast<int32_t>(v.get(name).get<double>());
}

std::string p_string(const picojson::value& v, const std::string& name)
{
  return v.get(name).get<std::string>();
}

AnimationFrame& Animation::current_frame()
{
  return frames[frame];
}

bool Animation::next(int64_t clock, double speed_multiplier)
{
  auto& f = frames[frame];
  const auto curr = clock::ticks();
  if ((curr - clock) / 1e3 <= f.duration / (speed * speed_multiplier)) {
    return false;
  }

  if (f.has_sound_effect) {
    if (sound_effect_loop || (!sound_effect_has_played && !sound_effect_loop)) {
      play_sound(f.sound_effect);
      sound_effect_has_played = true;
    }
  }

  switch (direction) {
    case AnimationDirection::forward:
      ++frame;
      if (frame < from) {
        frame = from;
      }

      if (frame > to) {
        if (hold_last_frame) {
          frame = to;
        } else {
          frame = from;
        }
      }
      break;

    case AnimationDirection::ping_pong:
      if (ping_backwards) {
        --frame;
      } else {
        ++frame;
      }

      if (frame > to) {
        frame = to - 1;
        ping_backwards = true;
      } else if (frame < from) {
        frame = from + 1;
        ping_backwards = false;
      }
      break;
  }

  return true;
}

bool Animation::register_sound_effect(int32_t frame, FileInfo wav, bool do_loop)
{
  if (frame < 0 || frame >= frames.size()) {
    return false;
  }

  auto& f = frames[frame];
  f.has_sound_effect = true;
  f.sound_effect = wav;
  sound_effect_loop = do_loop;

  return true;
}

std::shared_ptr<Sprite> Sprite::from_json(const FileInfo& path, bool reload)
{

  if (!reload) {
    auto in_cache = SPRITE_CACHE.find(path.file_relative);
    if (in_cache != SPRITE_CACHE.end()) {
      logger->info("Loading {} from cache", path.file_relative);
      return in_cache->second->clone(false);
    }
  }

  logger->info("Loading a new sprite from {}", path.file_relative);
  auto sprite = std::make_shared<Sprite>();
  auto input = path.open();

  if (!input) {
    return nullptr;
  }

  picojson::value doc;
  *input >> doc;

  auto frames = doc.get("frames").get<picojson::array>();
  auto meta = doc.get("meta");
  auto tags = meta.get("frameTags").get<picojson::array>();
  auto size = meta.get("size");

  sprite->width = p_int(size, "w");
  sprite->height = p_int(size, "h");
  sprite->speed = 1.0;

  fs::path relative_image_path(p_string(meta, "image"));
  fs::path image_path = path.file_dir / relative_image_path.filename();
  std::string image_cpath(image_path.string());

  logger->debug("Sprite texture is located at {}", image_path);

  auto exists = SURFACE_CACHE.find(image_path);
  if (exists != SURFACE_CACHE.end()) {
    sprite->surface = exists->second;
  } else {
    SDL_Surface* surface = IMG_Load(image_cpath.c_str());
    if (!surface) {
      logger->error("Sprite texture failed to load from {}: {}",
                    image_path);
      return nullptr;
    }
    SURFACE_CACHE[image_path].reset(surface, SDLDeleter());
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
    animation.sound_effect_has_played = false;
    animation.sound_effect_loop = false;
    animation.speed = 1.0f;

    logger->info("Adding animation {}", tag_name);

    if (direction == "forward") {
      animation.direction = AnimationDirection::forward;
    } else if (direction == "backward") {
      animation.direction = AnimationDirection::backward;
    } else if (direction == "pingpong") {
      if (from - to == 0) {
        animation.direction = AnimationDirection::forward;
      } else {
        animation.direction = AnimationDirection::ping_pong;
      }
    } else {
      std::cerr << direction << " is not a recognized animation direction\n";
      throw std::runtime_error("Unknown animation direction");
    }

    for (int32_t frame_num = from; frame_num <= to; ++frame_num) {
      AnimationFrame anim_frame;

      auto frame = frames.at(frame_num);
      auto frame_name = p_string(frame, "filename");
      const auto frame_size = frame.get("frame");
      const auto sprite_source_size = frame.get("spriteSourceSize");
      anim_frame.x = p_int(frame_size, "x") + p_int(sprite_source_size, "x");
      anim_frame.y = p_int(frame_size, "y") + p_int(sprite_source_size, "y");
      anim_frame.w = p_int(frame_size, "w");
      anim_frame.h = p_int(frame_size, "h");
      anim_frame.duration = p_int(frame, "duration");
      anim_frame.has_sound_effect = false;

      animation.speed = 1.0f;
      animation.frames.push_back(anim_frame);
    }

    sprite->animations[tag_name] = animation;
  }

  Animation* default_collision = nullptr;
  for (auto& anim : sprite->animations) {
    const auto& tag_name = anim.first;
    if (tag_name == "Collision-Default") {
      default_collision = &sprite->animations[tag_name];
      break;
    }
  }

  if (default_collision) {
    for (auto& anim : sprite->animations) {
      const auto& tag_name = anim.first;
      const auto find_it = tag_name.find("Collision-");
      if (find_it != std::string::npos) {
        continue;
      }
      logger->debug("Registered Default-Collision as collision for {}", tag_name);
      sprite->collision_frame_lut[tag_name] = default_collision;
    }
  }

  for (auto& anim : sprite->animations) {
    const auto& tag_name = anim.first;
    const auto find_it = tag_name.find("Collision-");
    if (find_it == std::string::npos) {
      continue;
    }
    auto ref_name = tag_name.substr(tag_name.find('-') + 1);
    if (ref_name == "Default") {
      continue;
    }
    auto found = sprite->animations.find(ref_name);
    if (found == sprite->animations.end()) {
      logger->error("Collision found for {}, but no such animation exist.", ref_name);
      return nullptr;
    }
    auto found_animation = &sprite->animations[tag_name];
    logger->debug("Registered {} as collision for {}", found_animation->name, ref_name);
    sprite->collision_frame_lut[ref_name] = found_animation;
  }

  sprite->current_animation = nullptr;
  sprite->show_collision_frame = false;
  sprite->last_frame_tick = clock::ticks();
  sprite->scale = 1.0;
  sprite->flip_x = false;
  sprite->flip_y = false;
  sprite->rotation_deg = 0.0;
  sprite->absolute_positioning = false;
  sprite->blend_mode = SDL_BLENDMODE_BLEND;
  sprite->path = path;
  sprite->render_in_foreground = false;
  sprite->set_animation("Idle");

  SPRITE_CACHE[path.file_relative] = sprite;

  return sprite->clone(false);
}

void Sprite::render(Renderer* renderer)
{
  if (!texture) {
    const auto exists = TEXTURE_CACHE.find(surface);
    if (exists == TEXTURE_CACHE.end()) {
      TEXTURE_CACHE[surface].reset(renderer->create_texture(surface), SDLDeleter());
      texture = TEXTURE_CACHE[surface];
    } else {
      texture = exists->second;
    }
    SDL_SetTextureBlendMode(texture.get(), blend_mode);
  }

  if (current_collision->name != current_animation->name) {
    current_collision->next(last_frame_tick, speed);
  }

  if (current_animation->next(last_frame_tick, speed)) {
    last_frame_tick = clock::ticks();
  }

  auto frame = current_animation->frames[current_animation->frame];

  if (show_collision_frame) {
    frame = current_collision->frames[current_collision->frame];
  }

  SDL_Rect src, dst;

  src.w = frame.w;
  src.h = frame.h;
  src.x = frame.x;
  src.y = frame.y;

  uint32_t H = renderer->window_size.h;

  dst.w = static_cast<int32_t>(frame.w * scale);
  dst.h = static_cast<int32_t>(frame.h * scale);
  dst.x = static_cast<int32_t>(x);
  dst.y = static_cast<int32_t>(y); // H - (y + dst.h));

  renderer->add_texture(texture, src, dst, rotation_deg, flip_x, flip_y, absolute_positioning, render_in_foreground);
}

bool Sprite::has_animation(const std::string& name)
{
  return animations.find(name) != animations.end();
}

Animation* Sprite::collision_animation(const std::string& name) const
{
  const auto found = collision_frame_lut.find(name);
  if (found == collision_frame_lut.end()) {
    return current_animation;
  }
  return found->second;
}

bool Sprite::set_animation(const std::string& name, bool hold_last_frame)
{
  if (current_animation && current_animation->name == name) {
    return true;
  }

  if (animations.find(name) == animations.end()) {
    return false;
  }

  if (current_animation) {
    current_animation->sound_effect_has_played = false;
  }

  current_animation = &animations[name];
  current_animation->frame = 0;
  current_animation->hold_last_frame = hold_last_frame;
  current_collision = this->collision_animation(name);
  current_collision->frame = 0;
  current_collision->hold_last_frame = hold_last_frame;
  return true;
}

std::shared_ptr<Sprite> Sprite::clone(bool reload)
{
  if (reload) {
    return Sprite::from_json(path, reload);
  }

  auto sprite = std::make_shared<Sprite>();

  sprite->width = width;
  sprite->height = height;
  sprite->speed = speed;
  sprite->surface = surface;
  sprite->animations = animations;
  sprite->current_animation = nullptr;

  Animation* default_collision = nullptr;
  for (auto& anim : sprite->animations) {
    const auto& tag_name = anim.first;
    if (tag_name == "Collision-Default") {
      default_collision = &sprite->animations[tag_name];
      break;
    }
  }

  if (default_collision) {
    for (auto& anim : sprite->animations) {
      const auto& tag_name = anim.first;
      const auto find_it = tag_name.find("Collision-");
      if (find_it != std::string::npos) {
        continue;
      }
      sprite->collision_frame_lut[tag_name] = default_collision;
    }
  }

  for (auto& anim : sprite->animations) {
    const auto& tag_name = anim.first;
    const auto find_it = tag_name.find("Collision-");
    if (find_it == std::string::npos) {
      continue;
    }
    auto ref_name = tag_name.substr(tag_name.find('-') + 1);
    if (ref_name == "Default") {
      continue;
    }
    auto found = sprite->animations.find(ref_name);
    if (found == sprite->animations.end()) {
      return nullptr;
    }
    auto found_animation = &sprite->animations[tag_name];
    sprite->collision_frame_lut[ref_name] = found_animation;
  }

  sprite->last_frame_tick = clock::ticks();
  sprite->scale = scale;
  sprite->flip_x = flip_x;
  sprite->flip_y = flip_y;
  sprite->rotation_deg = rotation_deg;
  sprite->absolute_positioning = absolute_positioning;
  sprite->blend_mode = blend_mode;
  sprite->path = path;
  sprite->render_in_foreground = render_in_foreground;
  sprite->set_animation(current_animation->name);

  return sprite;
}

bool Sprite::register_sound_effect(const std::string& name, int32_t frame, const FileInfo& wav, bool loop)
{
  const auto found = animations.find(name);
  if (found == animations.end()) {
    return false;
  }

  auto& f = found->second;

  if (frame == -1) {
    for (int32_t i = 0; i < f.frames.size(); ++i) {
      if (!found->second.register_sound_effect(i, wav, loop)) {
        return false;
      }
    }
  } else {
    return found->second.register_sound_effect(frame, wav, loop);
  }

  return true;
}


} // namespace raptr
