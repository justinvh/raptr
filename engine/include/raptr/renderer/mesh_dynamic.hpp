/*!
  /file static_mesh.hpp
  A static mesh is a simple Entity that is rather dumb, but it serves a purpose:
  something that the player can hit and attach to in some way or another.
*/
#pragma once

#include <memory>

#include <raptr/game/entity.hpp>
#include <raptr/common/filesystem.hpp>

namespace raptr
{
class Sprite;
class Game;

/*!
  A MeshDynamic is a simple collidable Entity in the world. It uses the Sprite
  to define its intersection and bounding box
*/
class MeshDynamic : public Entity
{
public:
  MeshDynamic();
  ~MeshDynamic() = default;
  MeshDynamic(const MeshDynamic&) = default;
  MeshDynamic(MeshDynamic&&) = default;
  MeshDynamic& operator=(const MeshDynamic&) = default;
  MeshDynamic& operator=(MeshDynamic&&) = default;

public:
  typedef MeshDynamicSpawnEvent SpawnEvent;

  /*!
    Returns the bounding box for this static mesh based on its sprite
    \return An rectangle containing the static mesh
  */
  std::vector<Rect> bbox() const override;

  /*!
    Generates a MeshDynamic object from a TOML configuration file
    /param path - The path to the TOML file
    /return An instance of the character if found
  */
  static std::shared_ptr<MeshDynamic> from_toml(const FileInfo& path);

  /*!
  */
  void render(Renderer* renderer) override;

  /*!
    The static mesh just needs to render. It is a simple function.
    \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  void think(std::shared_ptr<Game>& game) override;

  void serialize(std::vector<NetField>& list) override;

  bool deserialize(const std::vector<NetField>& fields) override;
};
} // namespace raptr
