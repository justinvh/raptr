#pragma once

#define SDL_MAIN_HANDLED

#include <memory>
#include <map>

#include "config.hpp"
#include "renderer.hpp"
#include "sound.hpp"
#include "controller.hpp"

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

public:
  bool is_init;
};

}
