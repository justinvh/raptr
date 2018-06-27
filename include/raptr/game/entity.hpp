/*!
  \file entity.hpp
  This module is a virtual class that represents an object in the world
*/
#pragma once

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
  /*!
    Returns the bounding box for this entity
    \return An rectangle containing the entity
  */
  virtual Rect bbox() const = 0;

  /*!
    Returns the unique id for this entity in the world
    \return a 32-bit signed integer representing a unique entity id
  */
  virtual int32_t id() const = 0;

  /*!
    Returns true if this entity intersects with another entity
    \param other - The entity that this entity will attempt to intersect against
    \return Whether an intersection occured
  */
  virtual bool intersects(const Entity* other) const = 0;

  /*!
    Convenience method to reset the last time think() was called
  */
  virtual void reset_think_delta()
  {
    last_think_time_ms = SDL_GetTicks();
  }

  /*!
    This method will determine how the entity interacts with the game.
    This include computing position, velocity, and acceleration updates; how the game elements
    interact with the entity; rendering it;
    \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  virtual void think(std::shared_ptr<Game> game) = 0;

  /*!
    How many seconds have happened since the last time think_delta was reset.
    \return A precise second value as a double
  */
  virtual double think_delta_s()
  {
    return (SDL_GetTicks() - last_think_time_ms) / 1000.0;
  }

  /*!
    How many milliseconds have happened since the last time think_delta was reset
    \return A precise number of milliseconds as a uint32_t
  */
  virtual uint32_t think_delta_ms()
  {
    return (SDL_GetTicks() - last_think_time_ms);
  }

  /*!
    Given the current position, velocity, and acceleration, where does this entity *want* to go in X
    \return The rectangle this entity *wants* to occupy in the X direction
  */
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

  /*!
    Given the current position, velocity, and acceleration, where does this entity *want* to go in Y
    \return The rectangle this entity *wants* to occupy in the Y direction
  */
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

  /*!
    Return the current position of the entity
    \return A double-precision point of the Entity position
  */
  virtual Point& position() { return pos_; }

  /*!
    Return the current velocity of the entity
    \return A double-precision point of the Entity velocity
  */
  virtual Point& velocity() { return vel_; }

  /*!
    Return the current acceleration of the entity
    \return A double-precision point of the Entity acceleration
  */
  virtual Point& acceleration() { return acc_; }

public:
  //! The current position 
  Point pos_;

  //! The current velocity
  Point vel_;

  //! The current acceleration
  Point acc_;

  //! How long the entity has been falling in milliseconds, if any
  uint32_t fall_time_ms;

  //! How long it has been since think() was called in milliseconds
  uint32_t last_think_time_ms;
};

}
