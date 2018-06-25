#include <SDL2/SDL_joystick.h>

#include "config.hpp"
#include "renderer.hpp"
#include "sound.hpp"
#include "game.hpp"
#include "character.hpp"
#include "sprite.hpp"
#include "controller.hpp"
#include "static_mesh.hpp"

namespace raptr {

bool Game::run()
{
  if (!is_init) {
    if (!this->init()) {
      std::cerr << "Failed to initialize the game.\n";
      return false;
    }
  }


  {
    std::shared_ptr<StaticMesh> mesh(new StaticMesh());
    mesh->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/platform.json");
    mesh->sprite->set_animation("Idle");
    mesh->sprite->x = 0;
    mesh->sprite->y = 100;
    mesh->_id = -2;

    SDL_Rect bbox = mesh->bbox();
    float min_bounds[2] = {bbox.x, bbox.y};
    float max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};
    last_known_entity_loc[mesh] = bbox;
    rtree.Insert(min_bounds, max_bounds, mesh.get());
    entities.push_back(mesh);
  }

  {
    std::shared_ptr<StaticMesh> mesh(new StaticMesh());
    mesh->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/platform.json");
    mesh->sprite->set_animation("Idle");
    mesh->sprite->x = 150;
    mesh->sprite->y = 300;
    mesh->_id = -3;

    SDL_Rect bbox = mesh->bbox();
    float min_bounds[2] = {bbox.x, bbox.y};
    float max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};
    last_known_entity_loc[mesh] = bbox;
    rtree.Insert(min_bounds, max_bounds, mesh.get());
    entities.push_back(mesh);
  }

  {
    std::shared_ptr<StaticMesh> mesh(new StaticMesh());
    mesh->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/platform.json");
    mesh->sprite->set_animation("Idle");
    mesh->sprite->x = 300;
    mesh->sprite->y = 500;
    mesh->_id = -4;

    SDL_Rect bbox = mesh->bbox();
    float min_bounds[2] = {bbox.x, bbox.y};
    float max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};
    last_known_entity_loc[mesh] = bbox;
    rtree.Insert(min_bounds, max_bounds, mesh.get());
    entities.push_back(mesh);
  }

  std::vector<std::shared_ptr<Character>> characters;
  {
    std::shared_ptr<Character> character(new Character());
    character->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/raptor.json");
    character->sprite->scale = 1.0;
    character->sprite->set_animation("Idle");
    character->last_think_time = SDL_GetTicks();
    character->walk_ups = 200;
    character->run_ups = 400;
    character->sprite->x = 0;
    character->sprite->y = 0;
    character->attach_controller(controllers.begin()->second);
    character->_id = -1;

    SDL_Rect bbox = character->bbox();
    float min_bounds[2] = {bbox.x, bbox.y};
    float max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};

    last_known_entity_loc[character] = bbox;
    rtree.Insert(min_bounds, max_bounds, character.get());

    characters.push_back(character);
    entities.push_back(character);
  }

  for (int i = 0; i < 0; ++i) {
    std::shared_ptr<Character> character(new Character());
    character->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/raptor.json");
    character->_id = i;
    //character->sprite->scale = 10.0;
    character->sprite->set_animation("Idle");
    character->last_think_time = SDL_GetTicks();
    character->walk_ups = 100;
    character->sprite->x = rand() % 1024 + 64;
    character->sprite->y = rand() % 1024 + 64;
    //character->attach_controller(controllers.begin()->second);

    SDL_Rect bbox = character->bbox();
    float min_bounds[2] = {bbox.x, bbox.y};
    float max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};

    last_known_entity_loc[character] = bbox;
    rtree.Insert(min_bounds, max_bounds, character.get());

    characters.push_back(character);
    entities.push_back(character);
  }

  SDL_Event e;
  while (true) {
    
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_CONTROLLERAXISMOTION) {
        int32_t controller_id = e.jdevice.which;
        controllers[controller_id]->process_event(e);
      }
    }

    for (auto& entity : entities) {
      entity->think(this->shared_from_this());

      SDL_Rect& old_bbox = last_known_entity_loc[entity];
      SDL_Rect new_bbox = entity->bbox();
      if (!SDL_RectEquals(&new_bbox, &old_bbox)) {
        float new_min_bounds[2] = {new_bbox.x, new_bbox.y};
        float new_max_bounds[2] = {new_bbox.x + new_bbox.w, new_bbox.y + new_bbox.h};
        float old_min_bounds[2] = {old_bbox.x, old_bbox.y};
        float old_max_bounds[2] = {old_bbox.x + old_bbox.w, old_bbox.y + old_bbox.h};
        rtree.Remove(old_min_bounds, old_max_bounds, entity.get());
        rtree.Insert(new_min_bounds, new_max_bounds, entity.get());
        last_known_entity_loc[entity] = new_bbox;
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
    SDL_Rect bbox;
    bool intersected;
  } condition_met;

  condition_met.check = entity;
  condition_met.intersected = false;
  condition_met.bbox = bbox;

  rtree.Search(min_bounds, max_bounds, [](Entity* found, void* context) -> bool {
    ConditionMet* condition = reinterpret_cast<ConditionMet*>(context);
    Entity* self = condition->check;
    if (self->id() == found->id()) {
      return true;
    }

    if (SDL_HasIntersection(&condition->bbox, &found->bbox())) {
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