#include <SDL.h>
#include <raptr/renderer/sprite.hpp>
#include <raptr/game/entity.hpp>
#include <raptr/game/character.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/common/clock.hpp>
#include <crossguid/guid.hpp>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr
{
Entity::Entity()
{
  auto g = xg::newGuid();
  is_dead = false;
  guid_ = g.bytes();
  fall_time_us = 0;
  collidable = true;
  think_rate_us = 0;
  gravity_ps2 = 0;
  last_think_time_us = clock::ticks();
}

const std::array<unsigned char, 16>& Entity::guid() const
{
  return guid_;
}

std::string Entity::guid_str() const
{
  std::string g;
  g.resize(16);
  std::copy(guid_.begin(), guid_.end(), g.begin());
  return g;
}

std::vector<Rect> Entity::want_position_x(int64_t delta_us)
{
  const double dt = delta_us / 1e6;
  Point pos = this->position_rel();
  const Point& vel = this->velocity_rel();
  pos.x += vel.x * dt;

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
  const auto delta_sec = delta_us / 1e6;
  Point pos = this->position_rel();
  const auto& vel = this->velocity_rel();
  const auto& acc = this->acceleration_rel();
  const auto delta_vel = delta_sec * acc.y;
  pos.y += vel.y * delta_sec + delta_vel / 2.0 * delta_sec;

  std::vector<Rect> rects = this->bbox();
  for (auto& r : rects) {
    r.x = pos.x;
    r.y = pos.y;
  }
  return rects;
}

bool Entity::is_player() const
{
  return dynamic_cast<const Character*>(this) != nullptr;
}

void Entity::add_velocity(double x_kmh, double y_kmh)
{
  vel_.x += x_kmh * kmh_to_ps;
  vel_.y += y_kmh * kmh_to_ps;
}

void Entity::add_acceleration(double x_ms2, double y_ms2)
{
  acc_.x += x_ms2 * meters_to_pixels;
  acc_.y += y_ms2 * meters_to_pixels;
}

bool Entity::intersects(const Entity* other) const
{
  if (other->guid() == this->guid()) {
    return false;
  }

  if (!other->collidable || !this->collidable) {
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
  }
  return this->intersect_fast(bbox);
}

bool Entity::intersects(const Entity* other, const Rect& bbox) const
{
  if (other->guid() == this->guid()) {
    return false;
  }

  if (!other->collidable || !this->collidable) {
    return false;
  }

  if (this->do_pixel_collision_test && other->do_pixel_collision_test) {
    return this->intersect_slow(other, bbox);
  }

  return this->intersect_fast(bbox);
}

bool Entity::intersect_slow(const Entity* other, const Rect& this_bbox) const
{
  if (is_dead || other->is_dead) {
    return false;
  }

  auto& this_sprite = this->sprite;
  const auto& this_surface = this_sprite->surface;
  const uint8_t* this_pixels = reinterpret_cast<uint8_t*>(this_surface->pixels);
  const int32_t this_bpp = this_surface->format->BytesPerPixel;

  auto& other_sprite = other->sprite;
  const auto& other_surface = other_sprite->surface;
  const uint8_t* other_pixels = reinterpret_cast<uint8_t*>(other_surface->pixels);
  const int32_t other_bpp = other_surface->format->BytesPerPixel;

  auto this_pos = this->position_abs();
  const auto other_pos = other->position_abs();
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
  auto cx0 = std::max(ax0, bx0);
  auto cx1 = std::min(ax1, bx1);
  auto cy0 = std::max(ay0, by0);
  auto cy1 = std::min(ay1, by1);

  // This pixel offsets
  auto tx0 = static_cast<int32_t>(cx0 - ax0);
  auto ty0 = static_cast<int32_t>(cy0 - ay0);

  // Other pixel offsets
  auto ox0 = static_cast<int32_t>(cx0 - bx0);
  auto oy0 = static_cast<int32_t>(cy0 - by0);

  auto th = static_cast<int32_t>(this_bbox.h - 1);
  auto oh = static_cast<int32_t>(other_frame->h - 1);

  auto w = static_cast<int32_t>(cx1 - cx0);
  auto h = static_cast<int32_t>(cy1 - cy0);

  for (int32_t x = 0; x < w; ++x) {
    for (int32_t y = 0; y < h; ++y) {
      int32_t this_idx = (this_frame->y + th - (ty0 + y)) * this_surface->pitch + (tx0 + x + this_frame->x) * this_bpp;
      int32_t other_idx = (other_frame->y + oh - (oy0 + y)) * other_surface->pitch + (ox0 + x + other_frame->x) *
          other_bpp;
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
  if (is_dead) {
    return false;
  }

  const auto& surface = sprite->surface;
  const uint8_t* pixels = reinterpret_cast<uint8_t*>(surface->pixels);
  const int32_t bpp = surface->format->BytesPerPixel;

  const auto pos = this->position_abs();
  auto& frame = collision_frame;

  // x0 position
  const auto x0_min = static_cast<int32_t>(std::max(other_box.x, pos.x));
  const auto x0_max = static_cast<int32_t>(std::min(other_box.x + other_box.w,
                                                    pos.x + frame->w - 1));

  // y0 position
  const auto y0_min = static_cast<int32_t>(std::max(other_box.y, pos.y));
  const auto y0_max = static_cast<int32_t>(std::min(other_box.y + other_box.h,
                                                    pos.y + frame->h - 1));

  int32_t th = surface->h;

  for (int32_t x0 = x0_min; x0 <= x0_max; ++x0) {
    for (int32_t y0 = y0_min; y0 <= y0_max; ++y0) {
      const int32_t idx = (frame->y + th - y0) * surface->pitch + (x0 + frame->x) * bpp;
      const uint8_t* px = pixels + idx;
      if (*px > 0) {
        return true;
      }
    }
  }

  return false;
}

void Entity::add_child(std::shared_ptr<Entity> child)
{
  child->set_parent(this->shared_from_this());
  children.push_back(child);
}

void Entity::remove_child(const std::shared_ptr<Entity>& child)
{
  const auto found = std::find(children.begin(), children.end(), child);
  if (found == children.end()) {
    return;
  }
  children.erase(found);
}

void Entity::set_parent(std::shared_ptr<Entity> new_parent)
{
  if (parent) {
    parent->remove_child(this->shared_from_this());
  }
  parent = new_parent;
}

bool Entity::intersect_fast(const Rect& other_box) const
{
  if (is_dead) {
    return false;
  }

  const auto& self_boxes = this->bbox();

  for (const auto& self_box : self_boxes) {
    if (SDL_HasIntersection(&self_box, &other_box)) {
      return true;
    }
  }

  return false;
}

void Entity::setup_lua_context(sol::state& state)
{
  state.new_usertype<Point>("Point",
    "x", &Point::x,
    "y", &Point::y
  );

  state.new_usertype<Entity>("Entity",
    "e", [](Entity* e) { return e->shared_from_this();  },
    "guid", &Entity::guid_str,
    "pos", &Entity::pos_,
    "vel", &Entity::vel_,
    "acc", &Entity::acc_,
    "gravity_ps2", &Entity::gravity_ps2,
    "add_child", &Entity::add_child,
    "remove_child", &Entity::remove_child,
    "set_parent", &Entity::set_parent,
    "children", &Entity::children,
    "parent", &Entity::parent
  );
}

} // namespace raptr
