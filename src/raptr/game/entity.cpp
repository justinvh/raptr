#include <SDL.h>
#include <raptr/game/entity.hpp>

namespace raptr {


void Entity::reset_think_delta()
{
  last_think_time_ms = SDL_GetTicks();
}

double Entity::think_delta_s()
{
  return (SDL_GetTicks() - last_think_time_ms) / 1000.0;
}

uint32_t Entity::think_delta_ms()
{
  return (SDL_GetTicks() - last_think_time_ms);
}

Rect Entity::want_position_x()
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

Rect Entity::want_position_y()
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

} // namespace raptr
