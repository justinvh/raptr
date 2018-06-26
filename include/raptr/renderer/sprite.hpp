#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_surface.h>

namespace raptr {

struct SDLDeleter {
  void operator()(SDL_Texture* p) const {
    SDL_DestroyTexture(p);
  }

  void operator()(SDL_Surface* p) const
  {
    if (p->refcount == 0) {
      SDL_FreeSurface(p);
    }
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
  bool hold_last_frame, ping_backwards;
  int32_t frame, from, to;
  std::vector<AnimationFrame> frames;
  AnimationDirection direction;
  float speed;

  AnimationFrame& current_frame()
  {
    return frames[frame];
  }

  bool next(uint32_t clock)
  {
    uint32_t curr = SDL_GetTicks();
    if ((curr - clock) <= frames[frame].duration / speed) {
      return false;
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
  void set_animation(const std::string& name, bool hold_last_frame = false);

public:
  AnimationTable animations;
  int32_t width, height;
  std::shared_ptr<SDL_Surface> surface;
  std::shared_ptr<SDL_Texture> texture;
  Animation* current_animation;
  float angle;
  bool flip_x, flip_y;
  bool solid;
  double x, y;
  double scale;
  uint32_t last_frame_tick;
};

}
