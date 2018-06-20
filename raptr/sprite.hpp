#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include <iostream>

#include <SDL2/SDL_surface.h>

namespace raptr {

struct SDLDeleter {
  void operator()(SDL_Texture* p) const {
    SDL_DestroyTexture(p);
  }

  void operator()(SDL_Surface* p) const
  {
    SDL_FreeSurface(p);
  }
};

struct AnimationFrame {
  std::string name;
  int32_t x, y, w, h;
  uint32_t duration;
};

enum class AnimationDirection {
  forward,
  backward,
  ping_pong
};

struct Animation {
  std::string name;
  int32_t frame, from, to;
  std::vector<AnimationFrame> frames;
  AnimationDirection direction;

  bool next(uint32_t clock)
  {
    uint32_t curr = SDL_GetTicks();
    if ((curr - clock) <= frames[frame].duration) {
      return false;
    }

    ++frame;
    if (frame < from) {
      frame = from;
    }

    if (frame > to) {
      frame = from;
    }

    return true;
  }

  void prev()
  {
    --frame;
    if (frame < from) {
      frame = to;
    }

    if (frame > to) {
      frame = to;
    }
  }
};

using AnimationTable = std::map<std::string, Animation>;

struct SpriteSheet {
  
  AnimationTable animations;
};

class Renderer;

class Sprite {
public:
  static std::shared_ptr<Sprite> from_json(const std::string& path);
  void render(std::shared_ptr<Renderer> renderer);
  void set_animation(const std::string& name);

public:
  AnimationTable animations;
  int32_t width, height;
  std::shared_ptr<SDL_Surface> surface;
  std::shared_ptr<SDL_Texture> texture;
  Animation* current_animation;
  int32_t x, y;
  double scale;
  uint32_t last_frame_tick;
};

}
