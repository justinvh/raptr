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
#include <raptr/common/filesystem.hpp>

namespace raptr {

class Sprite;
class Game;

/*!
  A StaticMesh is a simple collidable Entity in the world. It uses the Sprite
  to define its intersection and bounding box
*/
class StaticMesh : public Entity {
 public:
   StaticMesh();
   ~StaticMesh() = default;
   StaticMesh(const StaticMesh&) = default;
   StaticMesh(StaticMesh&&) = default;
   StaticMesh& operator=(const StaticMesh&) = default;
   StaticMesh& operator=(StaticMesh&&) = default;

 public:
  /*!
    Returns the bounding box for this static mesh based on its sprite
    \return An rectangle containing the static mesh
  */
  virtual std::vector<Rect> bbox() const;

  /*!
    Generates a StaticMesh object from a TOML configuration file
    /param path - The path to the TOML file
    /return An instance of the character if found
  */
  static std::shared_ptr<StaticMesh> from_toml(const FileInfo& path);

  /*!
    Returns true if this static mesh intersects with another entity
    \param other - The entity that this entity will attempt to intersect against
    \return Whether an intersection occured
  */
  virtual bool intersects(const Entity* other) const;
  virtual bool intersects(const Rect& bbox) const;

  /*!
    A fast bounding box intersection
    \param other - The entity this entity will attempt to intersect against
    \return Whether an intersection occured
  */
  bool intersect_fast(const Rect& bbox) const;

  /*!
    A slower per-pixel bounding box intersection test
    \param other - THe entity this entity will attempt to intersect against
    \return Whether an intersection occured
  */
  bool intersect_slow(const Rect& bbox) const;

  /*!
    The static mesh just needs to render. It is a simple function.
    \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  virtual void think(std::shared_ptr<Game>& game);

 public:
  //! If set, then the collisions will be done on the collision tagged mesh
  bool do_pixel_collision_test;
  
  //! The sprite that this Static Mesh uses to render and derive parameters
  std::shared_ptr<Sprite> sprite;
  AnimationFrame* collision_frame;

  //! A unique world id
  int32_t _id;
};

} // namespace raptr

