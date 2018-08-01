/*!
  \file entity.hpp
  This module is a virtual class that represents an object in the world
*/
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <sol.hpp>
#include <raptr/common/rect.hpp>
#include <raptr/network/snapshot.hpp>
#include <raptr/renderer/renderer.hpp>

namespace raptr
{

constexpr double magic_feel_good_number = 3;
constexpr double pixels_to_meters = 0.05 / magic_feel_good_number;
constexpr double meters_to_pixels = 1.0 / pixels_to_meters;
constexpr double pixels_to_kilometers = pixels_to_meters / 1e3;
constexpr double kilometers_to_pixels = 1.0 / pixels_to_kilometers;
constexpr double pixels_to_feet = pixels_to_meters * 3.28084;
constexpr double kmh_to_ps = kilometers_to_pixels / 3600.0;
constexpr double ms_to_ps = meters_to_pixels;

class Game;
class Sprite;
class Renderer;
struct AnimationFrame;

typedef std::array<unsigned char, 16> GUID;

/*!
  An Entity is any object in the world that can be interacted with by the player.
  This could be a static mesh, a character, or a simple platform. The idea is that
  an Entity is something you should care about as a player.
*/
class Entity : public RenderInterface,
               public Serializable, 
               public std::enable_shared_from_this<Entity>
{
public:
  Entity();
  Entity(const Entity&) = default;
  Entity(Entity&&) = default;
  Entity& operator=(const Entity&) = default;
  Entity& operator=(Entity&&) = default;
  virtual ~Entity() = default;

public:

  /*!
    Returns the bounding box for this entity
    \return An rectangle containing the entity
  */
  virtual std::vector<Rect> bbox() const = 0;

  /*!
    Returns the bounds of the entity
    */
  virtual std::vector<Bounds> bounds() const
  {
    std::vector<Rect> bboxes = this->bbox();
    std::vector<Bounds> all_bounds;

    for (const auto& bbox : bboxes) {
      Bounds bounds = {
        {bbox.x, bbox.y},
        {bbox.x + bbox.w, bbox.y + bbox.h}
      };
      all_bounds.push_back(bounds);
    }

    return all_bounds;
  }

  virtual void add_child(std::shared_ptr<Entity> child);
  virtual void remove_child(const std::shared_ptr<Entity>& child);

  virtual void set_parent(std::shared_ptr<Entity> new_parent);

  /*!
    Returns the unique id for this entity in the world
    \return a 32-bit signed integer representing a unique entity id
  */
  virtual const std::array<unsigned char, 16>& guid() const;

  virtual std::string guid_str() const;

  /*!
  Returns true if this static mesh intersects with another entity
  \param other - The entity that this entity will attempt to intersect against
  \return Whether an intersection occured
  */
  virtual bool intersects(const Entity* other) const;
  virtual bool intersects(const Rect& bbox) const;
  virtual bool intersects(const Entity* other, const Rect& bbox) const;

  /*!
  A fast bounding box intersection
  \param other - The entity this entity will attempt to intersect against
  \return Whether an intersection occured
  */
  virtual bool intersect_fast(const Rect& bbox) const;

  /*!
  A slower per-pixel bounding box intersection test
  \param other - THe entity this entity will attempt to intersect against
  \return Whether an intersection occured
  */
  virtual bool intersect_slow(const Rect& bbox) const;
  virtual bool intersect_slow(const Entity* other, const Rect& bbox) const;

  static void setup_lua_context(sol::state& state);

  /*!
    This method will determine how the entity interacts with the game.
    This include computing position, velocity, and acceleration updates; how the game elements
    interact with the entity; rendering it;
    \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  virtual void think(std::shared_ptr<Game>& game) = 0;

  /*!
    Given the current position, velocity, and acceleration, where does this entity *want* to go in X
    \return The rectangle this entity *wants* to occupy in the X direction
  */
  virtual std::vector<Rect> want_position_x(int64_t delta_us);

  /*!
    Given the current position, velocity, and acceleration, where does this entity *want* to go in Y
    \return The rectangle this entity *wants* to occupy in the Y direction
  */
  virtual std::vector<Rect> want_position_y(int64_t delta_us);

  /*!
    Return the current position of the entity
    \return A double-precision point of the Entity position
  */
  virtual Point& position_rel()
  {
    return pos_;
  }

  virtual const Point& position_rel() const
  {
    return pos_;
  }

  virtual Point position_abs() const
  {
    if (parent) {
      return parent->position_abs() + pos_;
    }
    return pos_;
  }

  /*!
    Return the current velocity of the entity
    \return A double-precision point of the Entity velocity
  */
  virtual Point& velocity_rel()
  {
    return vel_;
  }

  virtual const Point& velocity_rel() const
  {
    return vel_;
  }

  virtual Point velocity_abs() const
  {
    if (parent) {
      return parent->velocity_abs() + vel_;
    }
    return vel_;
  }

  /*!
    Return the current acceleration of the entity
    \return A double-precision point of the Entity acceleration
  */
  virtual Point& acceleration_rel()
  {
    return acc_;
  }

  virtual const Point& acceleration_rel() const
  {
    return acc_;
  }

  virtual Point acceleration_abs() const
  {
    if (parent) {
      return parent->acceleration_abs() + acc_;
    }
    return acc_;
  }

  virtual void add_velocity(double x_kmh, double y_kmh);

  virtual void add_acceleration(double x_ms2, double y_ms2);

  virtual bool is_player() const;

  void serialize(std::vector<NetField>& list) override = 0;

  bool deserialize(const std::vector<NetField>& fields) override = 0;

public:
  std::shared_ptr<Entity> parent;
  std::vector<std::shared_ptr<Entity>> children;

  double gravity_ps2;

  //! The name of the entity
  std::string name;

  //! The unique ID of the entity
  std::array<unsigned char, 16> guid_;

  //! Is collision possible?
  bool collidable;

  //! The current position
  Point pos_;

  //! The current velocity
  Point vel_;

  //! The current acceleration
  Point acc_;

  //! How long the entity has been falling in milliseconds, if any
  int64_t fall_time_us;

  //! Collision test per pixel
  bool do_pixel_collision_test;

  //! If true this entity is dead and won't collide anymore
  bool is_dead;

  //! The sprite that is used to render this character
  std::shared_ptr<Sprite> sprite;

  //! Specific frame used for collisions
  AnimationFrame* collision_frame;

  int64_t think_rate_us;
  int64_t last_think_time_us;
};
} // namespace raptr
