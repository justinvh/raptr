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

  {
    std::shared_ptr<Character> character(new Character());
    character->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/raptor.json");
    //character->sprite->scale = 5.0;
    character->sprite->set_animation("Idle");
    character->last_think_time = SDL_GetTicks();
    character->walk_ups = 500;
    character->sprite->x = rand() % 1024;
    character->sprite->y = 100;
    character->attach_controller(controllers.begin()->second);
    character->_id = -1;

    SDL_Rect bbox = character->bbox();
    float min_bounds[2] = {bbox.x, bbox.y};
    float max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};
    Entity* entity = static_cast<Entity*>(character.get());

    last_known_entity_loc[entity] = bbox;
    rtree.Insert(min_bounds, max_bounds, entity);

    characters.push_back(character);
  }

  for (int i = 0; i < 5; ++i) {
    std::shared_ptr<Character> character(new Character());
    character->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/raptor.json");
    character->_id = i;
    //character->sprite->scale = 10.0;
    character->sprite->set_animation("Idle");
    character->last_think_time = SDL_GetTicks();
    character->walk_ups = rand() % 1000;
    character->sprite->x = rand() % 1024;
    character->sprite->y = 100;
    //character->attach_controller(controllers.begin()->second);

    SDL_Rect bbox = character->bbox();
    float min_bounds[2] = {bbox.x, bbox.y};
    float max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};
    Entity* entity = static_cast<Entity*>(character.get());

    last_known_entity_loc[entity] = bbox;
    rtree.Insert(min_bounds, max_bounds, entity);

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
      Entity* entity = static_cast<Entity*>(character.get());

      SDL_Rect& old_bbox = last_known_entity_loc[entity];
      SDL_Rect new_bbox = character->bbox();
      if (!SDL_RectEquals(&new_bbox, &old_bbox)) {
        float new_min_bounds[2] = {new_bbox.x, new_bbox.y};
        float new_max_bounds[2] = {new_bbox.x + new_bbox.w, new_bbox.y + new_bbox.h};
        float old_min_bounds[2] = {old_bbox.x, old_bbox.y};
        float old_max_bounds[2] = {old_bbox.x + old_bbox.w, old_bbox.y + old_bbox.h};
        rtree.Remove(old_min_bounds, old_max_bounds, entity);
        rtree.Insert(new_min_bounds, new_max_bounds, entity);
      }
    }

    renderer->run_frame();
  }

  return true;
}

bool Game::entity_can_move_to(Entity* entity, const SDL_Rect& bbox)
{
  float min_bounds[2] = {bbox.x, bbox.y};
  float max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};

  struct ConditionMet {
    Entity* check;
    bool intersected;
  } condition_met;

  condition_met.check = entity;
  condition_met.intersected = false;

  rtree.Search(min_bounds, max_bounds, [](Entity* found, void* context) -> bool {
    ConditionMet* condition = reinterpret_cast<ConditionMet*>(context);
    Entity* self = condition->check;
    if (self->id() == found->id()) {
      return true;
    }

    if (self->intersects(found)) {
      condition->intersected = true;
      return false;
    }

    return true;
  }, reinterpret_cast<void*>(&condition_met));

  return !condition_met.intersected;
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