#include <SDL.h>
#include <raptr/renderer/sprite.hpp>
#include <raptr/game/entity.hpp>
#include <raptr/common/logging.hpp>

namespace { auto logger = raptr::_get_logger(__FILE__); };

namespace raptr {

Entity::Entity()
{
  auto g = xg::newGuid();
  guid_ = g.bytes();
  fall_time_us = 0;
}

const std::array<unsigned char, 16>& Entity::guid() const
{
  return guid_;
}

std::vector<Rect> Entity::want_position_x(int64_t delta_us)
{
  double dt = delta_us / 1e6;
  Point pos = this->position();
  const Point& vel = this->velocity();
  const Point& acc = this->acceleration();
  pos.x += vel.x * dt + 0.5 * acc.x * dt * dt;

  // Need a transformation matrix for this sort of stuff
  std::vector<Rect> rects = this->bbox();
  for (auto& r : rects) {
    r.x = pos.x;
    r.y = pos.y;
  }

  return rects;
}

std::vector<Rect> Entity::want_position_y(int64_t delta_us)
{
  double dt = delta_us / 1e6;
  Point pos = this->position();
  const Point& vel = this->velocity();
  const Point& acc = this->acceleration();
  pos.y += -vel.y * dt + -0.5 * acc.y * dt * dt;
  std::vector<Rect> rects = this->bbox();
  for (auto& r : rects) {
    r.x = pos.x;
    r.y = pos.y;
  }
  return rects;
}

bool Entity::intersects(const Entity* other) const
{
  if (other->guid() == this->guid()) {
    return false;
  }

  if (other->do_pixel_collision_test && do_pixel_collision_test) {
    for (auto& other_box : other->bbox()) {
      if (this->intersect_slow(other, other_box)) {
        return true;
      }
    }
    return false;
  }

  for (auto& self_box : this->bbox()) {
    for (auto& other_box : other->bbox()) {
      if (other->intersects(self_box) && this->intersects(other_box)) {
        return true;
      }
    }
  }

  return false;
}

bool Entity::intersects(const Rect& bbox) const
{
  if (this->do_pixel_collision_test) {
    return this->intersect_slow(bbox);
  } else {
    return this->intersect_fast(bbox);
  }
}

bool Entity::intersects(const Entity* other, const Rect& bbox) const
{
  if (this->do_pixel_collision_test) {
    return this->intersect_slow(other, bbox);
  } else {
    return this->intersect_fast(bbox);
  }
}

bool Entity::intersect_slow(const Entity* other, const Rect& this_bbox) const
{
  auto& this_sprite = this->sprite;
  const auto& this_surface = this_sprite->surface;
  const uint8_t* this_pixels = reinterpret_cast<uint8_t*>(this_surface->pixels);
  int32_t this_bpp = this_surface->format->BytesPerPixel;

  auto& other_sprite = other->sprite;
  const auto& other_surface = other_sprite->surface;
  const uint8_t* other_pixels = reinterpret_cast<uint8_t*>(other_surface->pixels);
  int32_t other_bpp = other_surface->format->BytesPerPixel;

  auto& this_pos = this->position();
  auto& other_pos = other->position();
  auto& this_frame = this->collision_frame;
  auto& other_frame = other->collision_frame;

  // Box 1
  double ax0 = this_bbox.x;
  double ax1 = ax0 + this_bbox.w - 1;
  double ay0 = this_bbox.y;
  double ay1 = ay0 + this_bbox.h - 1;

  // Box 2
  double bx0 = other_pos.x;
  double bx1 = other_pos.x + other_frame->w - 1;
  double by0 = other_pos.y;
  double by1 = other_pos.y + other_frame->h - 1;

  // Overlap
  double cx0 = std::max(ax0, bx0);
  double cx1 = std::min(ax1, bx1);
  double cy0 = std::max(ay0, by0);
  double cy1 = std::min(ay1, by1);

  // This pixel offsets
  int32_t tx0 = static_cast<int32_t>(cx0 - ax0);
  int32_t ty0 = static_cast<int32_t>(cy0 - ay0);

  // Other pixel offsets
  int32_t ox0 = static_cast<int32_t>(cx0 - bx0);
  int32_t oy0 = static_cast<int32_t>(cy0 - by0);

  int32_t w = static_cast<int32_t>(cx1 - cx0);
  int32_t h = static_cast<int32_t>(cy1 - cy0);

  for (int32_t x = 0; x < w; ++x) {
    for (int32_t y = 0; y < h; ++y) {
      int32_t this_idx = ((this_frame->y + ty0 + y) * this_surface->pitch) + (tx0 + x  + this_frame->x) * this_bpp;
      int32_t other_idx = ((other_frame->y + oy0 + y) * other_surface->pitch) + (ox0 + x + other_frame->x) * other_bpp;
      const uint8_t* this_px = this_pixels + this_idx;
      const uint8_t* other_px = other_pixels + other_idx;
      if (*this_px > 0 && *other_px > 0) {
        return true;
      }
    }
  }

  return false;
}

bool Entity::intersect_slow(const Rect& other_box) const
{
  const auto& surface = sprite->surface;
  const uint8_t* pixels = reinterpret_cast<uint8_t*>(surface->pixels);
  int32_t bpp = surface->format->BytesPerPixel;

  auto& pos = this->position();
  auto& frame = collision_frame;

  // x0 position
  int32_t x0_min = static_cast<int32_t>(std::max(other_box.x, pos.x));
  int32_t x0_max = static_cast<int32_t>(std::min(other_box.x + other_box.w,
    pos.x + frame->w - 1));

  // y0 position
  int32_t y0_min = static_cast<int32_t>(std::max(other_box.y, pos.y));
  int32_t y0_max = static_cast<int32_t>(std::min(other_box.y + other_box.h,
    pos.y + frame->h - 1));

  for (int32_t x0 = x0_min; x0 <= x0_max; ++x0) {
    for (int32_t y0 = y0_min; y0 <= y0_max; ++y0) {
      int32_t idx = ((frame->y + y0) * surface->pitch) + (x0 + frame->x) * bpp;
      const uint8_t* px = pixels + idx;
      if (*px > 0) {
        return true;
      }
    }
  }

  return false;
}

bool Entity::intersect_fast(const Rect& other_box) const
{
  const auto& self_boxes = this->bbox();

  for (const auto& self_box : self_boxes) {
    if (SDL_HasIntersection(&self_box, &other_box)) {
      return true;
    }
  }

  return false;
}


} // namespace raptr
