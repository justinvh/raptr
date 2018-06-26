#pragma once

#include <SDL2/SDL_rect.h>
#include <iostream>
#include "rect.hpp"

namespace raptr {

class Game;

class Entity {
public:
  virtual bool intersects(const Entity* other) const = 0;
  virtual int32_t id() const = 0;
  virtual Rect bbox() const = 0;
  virtual void think(std::shared_ptr<Game> game) = 0;

  virtual double think_delta_s()
  {
    return (SDL_GetTicks() - last_think_time_ms) / 1000.0;
  }

  virtual uint32_t think_delta_ms()
  {
    return (SDL_GetTicks() - last_think_time_ms);
  }

  virtual void reset_think_delta()
  {
    last_think_time_ms = SDL_GetTicks();
  }

  virtual Rect want_position_x()
  {
    double dt = this->think_delta_s();
    Point pos = this->position();
    const Point& vel = this->velocity();
    const Point& acc = this->acceleration();
    pos.x += vel.x * dt + 0.5 * acc.x * dt * dt;
    Rect rect = this->bbox();
    rect.x = pos.x;
    rect.y = pos.y;
    return rect;
  }

  virtual Rect want_position_y()
  {
    double dt = this->think_delta_s();
    Point pos = this->position();
    const Point& vel = this->velocity();
    const Point& acc = this->acceleration();
    pos.y += vel.y * dt + 0.5 * acc.y * dt * dt;
    Rect rect = this->bbox();
    rect.x = pos.x;
    rect.y = pos.y;
    return rect;
  }

  virtual Point& position() { return pos_; }
  virtual Point& velocity() { return vel_; }
  virtual Point& acceleration() { return acc_; }

public:
  Point pos_, vel_, acc_;
  uint32_t fall_time_ms;
  uint32_t last_think_time_ms;
};

}
