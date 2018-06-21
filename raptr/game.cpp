#include <SDL2/SDL_joystick.h>

#include "config.hpp"
#include "renderer.hpp"
#include "sound.hpp"
#include "game.hpp"
#include "character.hpp"
#include "sprite.hpp"
#include "controller.hpp"

namespace raptr {

bool Game::run()
{
  if (!is_init) {
    if (!this->init()) {
      std::cerr << "Failed to initialize the game.\n";
      return false;
    }
  }

  std::vector<std::shared_ptr<Character>> characters;

  for (int i = 0; i < 100; ++i) {
    std::shared_ptr<Character> character(new Character());
    character->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/raptor.json");
    //character->sprite->scale = 10.0;
    character->sprite->set_animation("Idle");
    character->last_think_time = SDL_GetTicks();
    character->walk_ups = rand() % 1000;
    character->sprite->x = rand() % 1024;
    character->sprite->y = rand() % 1024;
    character->attach_controller(controllers.begin()->second);
    characters.push_back(character);
  }

  SDL_Event e;
  while (true) {
    
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_JOYBUTTONUP || e.type == SDL_JOYBUTTONDOWN ||
          e.type == SDL_JOYAXISMOTION || e.type == SDL_JOYHATMOTION) 
      {
        int32_t controller_id = e.jdevice.which;
        controllers[controller_id]->process_event(e);
      }
    }

    for (auto& character : characters) {
      character->think(this->shared_from_this());
    }

    renderer->run_frame();
  }

  return true;
}

bool Game::init()
{
  config.reset(new Config());

  if (!this->init_renderer()) {
    std::cerr << "Failed to initialize renderer\n";
    return false;
  }

  if (!this->init_sound()) {
    std::cerr << "Failed to initialize sound\n";
    return false;
  }

  if (!this->init_controllers()) {
    std::cerr << "Failed to initialize controllers\n";
    return false;
  }

  return true;
}

bool Game::init_controllers()
{
  if (SDL_NumJoysticks() < 1) {
    std::cerr << "There are no controllers connected. What's the point of playing?\n";
    return false;
  }

  for (int32_t i = 0; i < SDL_NumJoysticks(); ++i) {
    if (!SDL_IsGameController(i)) {
      continue;
    }

    auto controller = Controller::open(i);
    controllers[controller->id()] = controller;
  }

  return !controllers.empty();
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