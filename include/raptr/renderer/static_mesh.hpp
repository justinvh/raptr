/*!
  /file static_mesh.hpp
  A static mesh is a simple Entity that is rather dumb, but it serves a purpose:
  something that the player can hit and attach to in some way or another.
*/
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <raptr/game/entity.hpp>

namespace raptr {

class Sprite;
class Game;

/*!
  A StaticMesh is a simple collidable Entity in the world. It uses the Sprite
  to define its intersection and bounding box
*/
class StaticMesh : public Entity {
public:
  /*!
  Returns the bounding box for this static mesh based on its sprite
  \return An rectangle containing the static mesh
  */
  virtual Rect bbox() const;

  /*!
  Returns the unique id for this static mesh in the world
  \return a 32-bit signed integer representing a unique static mesh id
  */
  virtual int32_t id() const;

  /*!
  Returns true if this static mesh intersects with another entity
  \param other - The entity that this entity will attempt to intersect against
  \return Whether an intersection occured
  */
  virtual bool intersects(const Entity* other) const;

  /*!
  The static mesh just needs to render. It is a simple function.
  \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  virtual void think(std::shared_ptr<Game> game);

public:
  //! The sprite that this Static Mesh uses to render and derive parameters
  std::shared_ptr<Sprite> sprite;

  //! A unique world id
  int32_t _id;
};


}
