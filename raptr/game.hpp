#pragma once

#define SDL_MAIN_HANDLED

#include <memory>
#include <map>

#include "config.hpp"
#include "renderer.hpp"
#include "sound.hpp"
#include "controller.hpp"
#include "rtree.hpp"
#include "entity.hpp"

namespace raptr {

class Game : public std::enable_shared_from_this<Game> {
private:
  Game() = default;

public:
  ~Game() = default;
  Game(const Game&) = delete;
  Game(Game&&) = default;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&) = default;

  static std::shared_ptr<Game> create()
  {
    return std::shared_ptr<Game>(new Game());
  }

  bool entity_can_move_to(Entity* entity, const SDL_Rect& bbox);

  bool run();

private:
  bool init();
  bool init_renderer();
  bool init_sound();
  bool init_controllers();


public:
  std::shared_ptr<Renderer> renderer;
  std::map<int32_t, std::shared_ptr<Controller>> controllers;
  std::shared_ptr<Sound> sound;
  std::shared_ptr<Config> config;
  std::vector<std::shared_ptr<Entity>> entities;
  std::map<std::shared_ptr<Entity>, SDL_Rect> last_known_entity_loc;
  RTree<Entity*, float, 2> rtree;

public:
  bool is_init;
};

}
