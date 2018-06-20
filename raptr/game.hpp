#pragma once

#define SDL_MAIN_HANDLED

#include <memory>

#include "config.hpp"
#include "renderer.hpp"
#include "sound.hpp"

namespace raptr {

class Game {
public:
  Game() = default;
  ~Game() = default;
  Game(const Game&) = delete;
  Game(Game&&) = default;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&) = default;

  bool run();

private:
  bool init();
  bool init_renderer();
  bool init_sound();

public:
  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<Sound> sound;
  std::shared_ptr<Config> config;

public:
  bool is_init;
};

}
