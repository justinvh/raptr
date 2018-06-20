#include "config.hpp"
#include "renderer.hpp"
#include "sound.hpp"
#include "game.hpp"
#include "sprite.hpp"

namespace raptr {

bool Game::run()
{
  if (!is_init) {
    this->init();
  }

  auto sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/raptor.json");
  sprite->scale = 5.0;

  sprite->set_animation("Walk Forward");


  SDL_Event e;
  while (true) {
    renderer->run_frame();

    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        break;
      } else if (e.button.button == SDL_BUTTON_LEFT) {
        sprite->x += 1;
      }
    }

    sprite->render(renderer);
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