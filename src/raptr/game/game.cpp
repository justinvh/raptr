#include <SDL_joystick.h>

#include <chrono>
#include <thread>
#include <memory>
#include <vector>

#include <raptr/config.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/sound/sound.hpp>
#include <raptr/game/game.hpp>
#include <raptr/game/character.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/renderer/static_mesh.hpp>
#include <raptr/common/filesystem.hpp>
#include <raptr/common/logging.hpp>

macro_enable_logger();

namespace raptr {

std::shared_ptr<Game> Game::create(const fs::path& game_root)
{
  fs::path full_path = fs::absolute(game_root);
  return std::shared_ptr<Game>(new Game(full_path));
}

Game::~Game()
{
  SDL_Quit();
}

bool Game::run()
{
  if (!is_init) {
    if (!this->init()) {
      std::cerr << "Failed to initialize the game.\n";
      return false;
    }
  }

  {
    auto mesh = StaticMesh::from_toml(filesystem->path("staticmeshes/fire.toml"));
    if (!mesh) {
      logger->error("Failed to load fire static mesh");
      return false;
    }
    mesh->sprite->x = 0;
    mesh->sprite->y = 250;
    mesh->_id = -40;
    Rect bbox = mesh->bbox();
    Bounds bounds = mesh->bounds();
    last_known_entity_loc[mesh] = bbox;
    rtree.Insert(bounds.min, bounds.max, mesh.get());
    entities.push_back(mesh);
  }

  {
    auto character_raptr = Character::from_toml(filesystem->path("characters/raptr.toml"));
    if (!character_raptr) {
      logger->error("Failed to load raptr character");
      return false;
    }

    entities.push_back(character_raptr);
    character_raptr->attach_controller(controllers.begin()->second);
    character_raptr->_id = -1;

    Rect bbox = character_raptr->bbox();
    Bounds bounds = character_raptr->bounds();

    last_known_entity_loc[character_raptr] = bbox;
    rtree.Insert(bounds.min, bounds.max, character_raptr.get());
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
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

  if (!this->init_filesystem()) {
    logger->error("Failed to initialize filesystem. "
                  "Are you sure {} is the game path?", game_root);
    return false;
  }

  config.reset(new Config());
  gravity = 9.8;

  if (!this->init_renderer()) {
    logger->error("Failed to initialize renderer");
    return false;
  }

  if (!this->init_sound()) {
    logger->error("Failed to initialize sound");
    return false;
  }

  if (!this->init_controllers()) {
    logger->error("Failed to initialize controllers");
    return false;
  }

  return true;
}

bool Game::init_controllers()
{
  if (SDL_NumJoysticks() < 1) {
    logger->error("There are no controllers connected. What's the point of playing?");
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
  return true;
}

bool Game::init_filesystem()
{
  logger->info("Registering the game root as {}", game_root);

  if (!fs::exists(game_root)) {
    logger->error("{} does not exist!", game_root);
    return false;
  }

  filesystem.reset(new Filesystem(game_root));
  return true;
}

} // namespace raptr
