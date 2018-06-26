#include <SDL_joystick.h>
#include <chrono>
#include <thread>

#include <raptr/config.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/sound/sound.hpp>
#include <raptr/game/game.hpp>
#include <raptr/game/character.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/renderer/static_mesh.hpp>

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
    mesh->sprite = Sprite::from_json("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/textures/fire.json");
    mesh->sprite->set_animation("Idle");
    mesh->sprite->x = 0;
    mesh->sprite->y = 250;
    mesh->_id = -40;
    Rect bbox = mesh->bbox();
    double min_bounds[2] = {bbox.x, bbox.y};
    double max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};
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
    character->walk_speed = 100;
    character->run_speed = 300;
    character->sprite->x = 0;
    character->sprite->y = 0;
    character->jump_vel = 300;

    auto& acc = character->acceleration();
    acc.y = 9.8;

    character->attach_controller(controllers.begin()->second);
    character->_id = -1;

    Rect bbox = character->bbox();
    double min_bounds[2] = {bbox.x, bbox.y};
    double max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};

    last_known_entity_loc[character] = bbox;
    rtree.Insert(min_bounds, max_bounds, character.get());

    characters.push_back(character);
    entities.push_back(character);
  }

  SDL_Event e;
  uint32_t last_game_tick = SDL_GetTicks();
  while (true) {

    if ((SDL_GetTicks() - last_game_tick) < 1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_CONTROLLERAXISMOTION ||
          e.type == SDL_CONTROLLERBUTTONDOWN ||
          e.type == SDL_CONTROLLERBUTTONUP)
      {
        int32_t controller_id = e.jdevice.which;
        controllers[controller_id]->process_event(e);
      } else if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F1) {
        renderer->toggle_fullscreen();
      }
    }

    for (auto& entity : entities) {
      entity->think(this->shared_from_this());

      Rect& old_bbox = last_known_entity_loc[entity];
      Rect new_bbox = entity->bbox();
      if (!SDL_RectEquals(&new_bbox, &old_bbox)) {
        double new_min_bounds[2] = {new_bbox.x, new_bbox.y};
        double new_max_bounds[2] = {new_bbox.x + new_bbox.w, new_bbox.y + new_bbox.h};
        double old_min_bounds[2] = {old_bbox.x, old_bbox.y};
        double old_max_bounds[2] = {old_bbox.x + old_bbox.w, old_bbox.y + old_bbox.h};
        rtree.Remove(old_min_bounds, old_max_bounds, entity.get());
        rtree.Insert(new_min_bounds, new_max_bounds, entity.get());
        last_known_entity_loc[entity] = new_bbox;
      }
    }

    renderer->run_frame();
    last_game_tick = SDL_GetTicks();
  }

  return true;
}

bool Game::entity_can_move_to(Entity* entity, const Rect& bbox)
{
  double min_bounds[2] = {bbox.x, bbox.y};
  double max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};

  struct ConditionMet {
    Entity* check;
    Rect bbox;
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
  gravity = 9.8;

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