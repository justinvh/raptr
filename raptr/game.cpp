#include "config.hpp"
#include "renderer.hpp"
#include "sound.hpp"
#include "game.hpp"
#include "character.hpp"
#include "sprite.hpp"

namespace raptr {

bool Game::run()
{
  if (!is_init) {
    this->init();
  }

  std::vector<Character> characters;

  for (int i = 0; i < 1; i++) {
    auto character = Character();
    character.sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/raptor.json");
    character.sprite->scale = 5.0;
    character.sprite->set_animation("Idle");
    character.last_think_time = SDL_GetTicks();
    character.walk_ups = 10.0;
    character.nx = character.sprite->x;
    character.ny = character.sprite->y;
    characters.push_back(character);
  }

  std::shared_ptr<Game> game(this);

  SDL_Event e;
  while (true) {
    renderer->run_frame();

    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        break;
      }
    }

    for (auto c : characters) {
      int direction = rand() % 4;
      if (c.is_moving()) {
        c.think(game);
        continue;
      }

      if (direction == 0) {
        c.walk_right(5);
      } else if (direction == 1) {
        c.walk_left(5);
      } else if (direction == 2) {
        c.walk_up(5);
      } else if (direction == 3) {
        c.walk_down(5);
      }

      c.think(game);

      if (c.nx < 0) {
        c.nx = 0;
      } else if (c.nx > 1000) {
        c.nx = 1000;
      }

      if (c.ny > 1000) {
        c.ny = 1000;
      } else if (c.ny < 0) {
        c.ny = 0;
      }
    }
  }

  return true;
}

bool Game::init()
{
  config.reset(new Config());
  this->init_renderer();
  this->init_sound();
  return true;
}

bool Game::init_renderer()
{
  renderer.reset(new Renderer());
  renderer->init(config);
  return true;
}

bool Game::init_sound()
{
  sound.reset(new Sound());
  sound->init(config);
  return true;
}

}