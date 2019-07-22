  /*!
  /file actor.hpp
  An actor is a simple Entity that is rather dumb, but it serves a purpose:
  something that the player can hit and attach to in some way or another.
*/
#pragma once

#include <memory>
#include <sol/sol.hpp>

#include <raptr/game/entity.hpp>
#include <raptr/common/filesystem.hpp>

namespace raptr
{
class Sprite;
class Game;

/*!
  An actor is a simple collidable Entity in the world. It uses the Sprite
  to define its intersection and bounding box
*/
class Actor : public Entity
{
public:
  Actor();
  ~Actor() = default;
  Actor(const Actor&) = default;
  Actor(Actor&&) = default;
  Actor& operator=(const Actor&) = default;
  Actor& operator=(Actor&&) = default;

public:
  typedef ActorSpawnEvent SpawnEvent;

  static void setup_lua_context(sol::state& state);

  /*!
    Returns the bounding box for this static mesh based on its sprite
    \return An rectangle containing the static mesh
  */
  Rect bbox() const override;

  /*!
    Generates a MeshStatic object from a TOML configuration file
    /param path - The path to the TOML file
    /return An instance of the character if found
  */
  static std::shared_ptr<Actor> from_toml(const FileInfo& path);

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

  bool is_scripted;
  sol::state lua;
  FileInfo lua_script_fileinfo;
  std::string lua_script;
};
} // namespace raptr
