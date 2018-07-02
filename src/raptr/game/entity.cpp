#include <SDL.h>
#include <raptr/game/entity.hpp>
#include <raptr/common/logging.hpp>

namespace { auto logger = raptr::_get_logger(__FILE__); };

namespace raptr {

int32_t Entity::global_id = 0;

Entity::Entity()
{
  id_ = ++global_id;
  fall_time_ms = 0;
  logger->info("Registering entity with id {}", id_);
}

int32_t Entity::id() const
{
  return id_;
}

Rect Entity::want_position_x(int64_t delta_ms)
{
  double dt = delta_ms / 1000.0;
  Point pos = this->position();
  const Point& vel = this->velocity();
  const Point& acc = this->acceleration();
  pos.x += vel.x * dt + 0.5 * acc.x * dt * dt;
  Rect rect = this->bbox();
  rect.x = pos.x;
  rect.y = pos.y;
  return rect;
}

Rect Entity::want_position_y(int64_t delta_ms)
{
  double dt = delta_ms / 1000.0;
  Point pos = this->position();
  const Point& vel = this->velocity();
  const Point& acc = this->acceleration();
  pos.y += -vel.y * dt + -0.5 * acc.y * dt * dt;
  Rect rect = this->bbox();
  rect.x = pos.x;
  rect.y = pos.y;
  return rect;
}

} // namespace raptr
