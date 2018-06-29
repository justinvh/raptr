/*!
  \file entity.hpp
  This module is a virtual class that represents an object in the world
*/
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <raptr/common/rect.hpp>

namespace raptr {

class Game;

/*!
  An Entity is any object in the world that can be interacted with by the player.
  This could be a static mesh, a character, or a simple platform. The idea is that
  an Entity is something you should care about as a player.
*/
class Entity {
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
  virtual Rect bbox() const = 0;

  /*!
    Returns the bounds of the entity
    */
  virtual Bounds bounds() const
  {
    Rect bbox = this->bbox();
    Bounds bounds = {
      {bbox.x, bbox.y},
      {bbox.x + bbox.w, bbox.y + bbox.h}
    };
    return bounds;
  }

  /*!
    Returns the unique id for this entity in the world
    \return a 32-bit signed integer representing a unique entity id
  */
  virtual int32_t id() const;

  /*!
    Returns true if this entity intersects with another entity
    \param other - The entity that this entity will attempt to intersect against
    \return Whether an intersection occured
  */
  virtual bool intersects(const Entity* other) const = 0;

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
  virtual Rect want_position_x(int64_t delta_ms);

  /*!
    Given the current position, velocity, and acceleration, where does this entity *want* to go in Y
    \return The rectangle this entity *wants* to occupy in the Y direction
  */
  virtual Rect want_position_y(int64_t delta_ms);

  /*!
    Return the current position of the entity
    \return A double-precision point of the Entity position
  */
  virtual Point& position() { return pos_; }
  virtual const Point& position() const { return pos_; }

  /*!
    Return the current velocity of the entity
    \return A double-precision point of the Entity velocity
  */
  virtual Point& velocity() { return vel_; }
  virtual const Point& velocity() const { return vel_; }

  /*!
    Return the current acceleration of the entity
    \return A double-precision point of the Entity acceleration
  */
  virtual Point& acceleration() { return acc_; }
  virtual const Point& acceleration() const { return acc_; }

 public:
  //! The name of the entity
   std::string name;

  //! The unique ID of the entity
  int32_t id_;

  //! The current position
  Point pos_;

  //! The current velocity
  Point vel_;

  //! The current acceleration
  Point acc_;

  //! How long the entity has been falling in milliseconds, if any
  uint32_t fall_time_ms;

 private:
  //! A simple ID counter
  static int32_t global_id;
};

} // namespace raptr
