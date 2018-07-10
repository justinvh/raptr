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
   typedef StaticMeshSpawnEvent SpawnEvent;

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
  */
  virtual void render(Renderer* renderer);

  /*!
    The static mesh just needs to render. It is a simple function.
    \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  virtual void think(std::shared_ptr<Game>& game);

  virtual void serialize(std::vector<NetField>& list);

  virtual bool deserialize(const std::vector<NetField>& fields);
};

} // namespace raptr

