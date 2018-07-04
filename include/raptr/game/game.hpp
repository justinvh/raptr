/*!
  \file game.hpp
  This is the monolothic component of handling event dispatching and glueing the
  pieces all together.
*/
#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <map>
#include <memory>
#include <vector>
#include <chrono>

#include <raptr/common/filesystem.hpp>
#include <raptr/common/rtree.hpp>
#include <raptr/common/rect.hpp>
#include <raptr/common/clock.hpp>

namespace raptr {

class Config;
class Controller;
class Entity;
class Renderer;
class Sound;

/*!
  The Game is a class that ties together the Renderer, Sound, Input, and Entities
  into one cohesive interaction. It can be thought of the main loop of the application
  with additional utilities and conveniences. 
*/
class Game : public std::enable_shared_from_this<Game> {
 private:
  //! The class should be created using Game::create as it inherits from enable_shared_from_this
  Game(const fs::path& game_root_)
    : is_init(false), game_root(game_root_) { }

 public:
  ~Game();
  Game(const Game&) = delete;
  Game(Game&&) = default;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&) = default;

 public:
  /*!
    This should be used when creating a Game. 
  */
  static std::shared_ptr<Game> create(const fs::path& game_root);

 public:
  /*!
    Returns true if a given entity can teleport to a region defined by a bounding box
    \param entity - The entity that is trying to teleport
    \param bbox - The bounding box that the entity wants to teleport to
    \return The first entity that the object intersected or null if nothing
  */
  std::shared_ptr<Entity> intersect_world(Entity* entity, const Rect& bbox);

  /*!
    Run the game and manage maintaining a healthy FPS
    \return Whether or not the game successfully ran
  */
  bool run();

 private:
  /*!
    Top-level function to call all other init functions
    \return Whether all init functions passed or not
  */
  bool init();

  /*!
    Initialize and find controllers the game can use
    \return Whether a controller was found
  */
  bool init_controllers();

  /*!
    Initialize the filesystem. This allows us to quickly access 
    files relative to the game directory.
    \return Whether the filesystem could be constructed
  */
  bool init_filesystem();

  /*!
    Initialize the renderer and setup the screen for game-time
    \return Whether the renderer could be initialized
  */
  bool init_renderer();

  /*!
    Initialize the sound engine on a separate thread
    \return Whether the sound engine could be initialized
  */
  bool init_sound();

 public:
  //! Global configuration loaded from a etc/raptr.ini
  std::shared_ptr<Config> config;

  //! Instance of the renderer that is being used by the game
  std::shared_ptr<Renderer> renderer;

  //! Instance of the sound engine being used
  std::shared_ptr<Sound> sound;

  //! The filesystem object for accessing files to the game
  FileInfo game_path;

  //! A mapping of controller device ID to controller instances
  std::map<int32_t, std::shared_ptr<Controller>> controllers;

  //! A list of all loaded entities in the game
  std::vector<std::shared_ptr<Entity>> entities;

  //! A mapping of entity IDs to entities
  std::map<int32_t, std::shared_ptr<Entity>> entity_lut;

  //! A map to find what the last known location of an entity was
  std::map<std::shared_ptr<Entity>, Point> last_known_entity_pos;
  std::map<std::shared_ptr<Entity>, std::vector<Bounds>> last_known_entity_bounds;

  //! The r-tree is used as a secondary test for bounding box interactions
  RTree<Entity*, double, 2> rtree;

  //! The root of the game folders to extract
  fs::path game_root;

  //! The gravity of the world
  double gravity;

  //! The number of ms since the last frame
  int64_t frame_delta_us;

 public:
  //! If set, then all initialization has happened successfully
  bool is_init;
};

} // namespace raptr

