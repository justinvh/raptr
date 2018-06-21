#pragma once

#include <SDL2/SDL_rect.h>

namespace raptr {


class Entity {
public:
  virtual bool intersects(const Entity* other) const = 0;
  virtual int32_t id() const = 0;
  virtual SDL_Rect bbox() const = 0;
};

}
