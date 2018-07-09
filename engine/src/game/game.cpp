#include <SDL_joystick.h>

#include <chrono>
#include <thread>
#include <memory>
#include <vector>

#include <raptr/common/filesystem.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/config.hpp>
#include <raptr/game/character.hpp>
#include <raptr/game/game.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/renderer/parallax.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/renderer/static_mesh.hpp>
#include <raptr/sound/sound.hpp>
#include <raptr/ui/dialog.hpp>

namespace { auto logger = raptr::_get_logger(__FILE__); };

namespace raptr {

std::shared_ptr<Game> Game::create(const fs::path& game_root)
{
  fs::path full_path = fs::absolute(game_root);
  auto game = std::shared_ptr<Game>(new Game(full_path));
  game->is_headless = false;
  if (!game->is_init) {
    if (!game->init()) {
      logger->error("Failed to initialize the game");
      return nullptr;
    }
  }
  game->frame_last_time = clock::ticks();
  return game;
}

std::shared_ptr<Game> Game::create_headless(const fs::path& game_root)
{
  fs::path full_path = fs::absolute(game_root);
  auto game = std::shared_ptr<Game>(new Game(full_path));
  game->is_headless = true;
  if (!game->is_init) {
    if (!game->init()) {
      logger->error("Failed to initialize the game");
      return nullptr;
    }
  }
  logger->info("Running in headless mode");
  game->frame_last_time = clock::ticks();
  return game;
}

Game::~Game()
{
  SDL_Quit();
}

bool Game::poll_events()
{
  SDL_Event e;
  if (!SDL_PollEvent(&e)) {
    return false;
  }

  if (e.type == SDL_CONTROLLERAXISMOTION || e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP) {
    int32_t controller_id = e.jdevice.which;
    controllers[controller_id]->process_event(e);
  } else if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F1) {
    renderer->toggle_fullscreen();
  } else if (e.type == SDL_JOYAXISMOTION || e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP) {
    int32_t controller_id = e.jdevice.which;
    auto& controller = controllers[controller_id];
    if (!controller->is_gamepad()) {
      controller->process_event(e);
    }
  }
  return true;
}

bool Game::run_frame()
{
  if (!is_headless) {
    this->poll_events();
  }

  auto current_time_us = clock::ticks();
  if ((current_time_us - frame_last_time) < 100) {
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    return true;
  }

  frame_delta_us = (current_time_us - frame_last_time);

  for (auto& entity : entities) {
    entity->think(this->shared_from_this());

    const Point& old_point = last_known_entity_pos[entity];
    Point& new_point = entity->position();

    if (std::fabs(old_point.x - new_point.x) > 0.5 ||
      std::fabs(old_point.y - new_point.y) > 0.5) {
      for (auto& b : last_known_entity_bounds[entity]) {
        rtree.Remove(b.min, b.max, entity.get());
      }
      last_known_entity_bounds[entity] = entity->bounds();
      for (auto& b : entity->bounds()) {
        rtree.Insert(b.min, b.max, entity.get());
      }
      last_known_entity_pos[entity] = entity->position();
    }

    if (new_point.y > 1000) {
      new_point.y = -500;
    }
  }

  if (!is_headless) {
    renderer->run_frame();
  }

  frame_last_time = clock::ticks();
  return true;
}

bool Game::run()
{
  while (true) {
    if (!this->run_frame()) {
      return false;
    }
  }
  return true;
}

std::shared_ptr<Entity> Game::intersect_world(Entity* entity, const Rect& bbox)
{
  double min_bounds[2] = {bbox.x, bbox.y};
  double max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};

  struct ConditionMet {
    Entity* check;
    Rect bbox;
    Entity* found;
    bool intersected;
  } condition_met;

  condition_met.check = entity;
  condition_met.intersected = false;
  condition_met.bbox = bbox;
  condition_met.found = nullptr;

  rtree.Search(min_bounds, max_bounds, [](Entity* found, void* context) -> bool {
    ConditionMet* condition = reinterpret_cast<ConditionMet*>(context);
    Entity* self = condition->check;
    if (self->guid() == found->guid()) {
      return true;
    }

    bool has_intersection = false;
    if (self->do_pixel_collision_test && found->do_pixel_collision_test) {
      has_intersection = self->intersects(found, condition->bbox);
    } else {
      has_intersection = self->intersects(condition->bbox);
    }

    if (has_intersection) {
      condition->intersected = true;
      condition->found = found;
      return false;
    }

    return true;
  }, reinterpret_cast<void*>(&condition_met));

  if (condition_met.intersected) {
    return entity_lut[condition_met.found->guid()];
  }

  return nullptr;
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
  gravity = -4500;

  if (!this->init_controllers()) {
    logger->error("Failed to initialize controllers");
    return false;
  }

  if (!this->init_sound()) {
    logger->error("Failed to initialize sound");
    return false;
  }

  if (!this->init_renderer()) {
    logger->error("Failed to initialize renderer");
    return false;
  }

  if (!this->init_demo()) {
    logger->error("Failed to initialize the demo");
    return false;
  }

  return true;
}

bool Game::init_controllers()
{
  if (is_headless) {
    return true;
  }

  SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
  SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

  auto mapping_file = game_path.from_root(fs::path("controls") / "gamecontrollerdb.txt").file_path;
  auto mapping_file_str = mapping_file.string();
  SDL_GameControllerAddMappingsFromFile(mapping_file_str.c_str());

  int32_t num_gamepads = 0;
  for (int32_t i = 0; i < SDL_NumJoysticks(); ++i) {
    num_gamepads++;
  }

  if (num_gamepads == 0) {

    int32_t attempts = 60;
    logger->error("No controllers connected. Waiting {}s for you to go find a controller. Hurry up.", attempts);
    while (--attempts) {
      SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
      SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
      for (int32_t i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
          num_gamepads++;
        }
      }

      if (num_gamepads) {
        logger->info("Nice. Moving on.");
        break;
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));

      switch (attempts) {
        case 55:
          logger->error("Your controller has met a terrible fate, hasn't it? {}s left.", attempts);
          break;
        case 50:
          logger->error("The world darkens as our player scrambles for a controller.");
          break;
        case 45:
          logger->error("In the distance a soft cry is heard. That of our player's controller?");
          break;
        case 40:
          logger->error("At this point, we fear for the player. Not for his health, but his disorganization.");
          break;
        case 30:
          logger->error("Maybe it's not the player. Maybe they loaned their controller out. A true friend.");
          break;
        case 20:
          logger->error("Nonsense. They would have exited by now.");
          break;
        case 10:
          logger->error("The world darkens even more. There isn't much hope.");
          break;
        case 5:
          logger->error("I hope you tried. I really hope you did.");
          break;
      }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  if (num_gamepads == 0) {
    logger->error("Well, that's the most I can do. No controllers detected.");
    return false;
  }

  for (int32_t i = 0; i < SDL_NumJoysticks(); ++i) {
    logger->debug("Is {} a game controller? {}",
                  SDL_JoystickNameForIndex(i),
                  SDL_IsGameController(i) ? "Yes" : "No");

    auto controller = Controller::open(game_path, i);
    controllers[controller->id()] = controller;
  }

  return !controllers.empty();
}

bool Game::init_demo()
{
  // background test
  auto background = Parallax::from_toml(game_path.from_root("background/nightsky.toml"));
  if (background) {
    renderer->add_background(background);
  }

  auto foreground = Parallax::from_toml(game_path.from_root("foreground/nightsky.toml"));
  if (foreground) {
    renderer->add_foreground(foreground);
  }

  auto dialog = Dialog::from_toml(game_path.from_root("dialog/demo/dialog.toml"));
  //dialog->attach_controller(controllers.begin()->second);
  //dialog->start();

  {
    auto mesh = StaticMesh::from_toml(game_path.from_root("staticmeshes/demo.toml"));
    if (!mesh) {
      logger->error("Failed to load demo static mesh");
      return false;
    }
    auto& pos = mesh->position();
    pos.x = 0;
    pos.y = 0;

    const auto& bounds = mesh->bounds();
    last_known_entity_pos[mesh] = mesh->position();

    for (const auto& b : bounds) {
      rtree.Insert(b.min, b.max, mesh.get());
    }

    renderer->observing.push_back(mesh);
    entities.push_back(mesh);
    entity_lut[mesh->guid()] = mesh;
  }

  std::vector<std::shared_ptr<Entity>> characters;

  int64_t x_off = 800;
  for (auto controller : controllers) {
    auto character_raptr = Character::from_toml(game_path.from_root("characters/raptr.toml"));
    if (!character_raptr) {
      logger->error("Failed to load raptr character");
      return false;
    }
    auto& pos = character_raptr->position();
    pos.y = 0;
    pos.x = static_cast<double>(x_off);
    x_off += 64;

    character_raptr->flashlight = true;
    character_raptr->attach_controller(controller.second);

    auto all_bounds = character_raptr->bounds();
    last_known_entity_pos[character_raptr] = character_raptr->position();
    last_known_entity_bounds[character_raptr] = character_raptr->bounds();

    for (const auto& bounds : all_bounds) {
      rtree.Insert(bounds.min, bounds.max, character_raptr.get());
    }

    characters.push_back(character_raptr);
    entities.push_back(character_raptr);
    renderer->observing.push_back(character_raptr);
    entity_lut[character_raptr->guid()] = character_raptr;
  }

  /*
  x_off = 135;
  for (int i = 0; i < 2; ++i) {
  auto character_raptr = Character::from_toml(game_path.from_root("characters/raptr.toml"));
  if (!character_raptr) {
  logger->error("Failed to load raptr character");
  return false;
  }
  auto& pos = character_raptr->position();
  pos.y = 100;
  pos.x = x_off;
  x_off += 60;

  entities.push_back(character_raptr);
  characters.push_back(character_raptr);
  //character_raptr->attach_controller(controller.second);

  auto all_bounds = character_raptr->bounds();
  last_known_entity_pos[character_raptr] = character_raptr->position();
  last_known_entity_bounds[character_raptr] = character_raptr->bounds();

  for (const auto& bounds : all_bounds) {
  rtree.Insert(bounds.min, bounds.max, character_raptr.get());
  }

  entity_lut[character_raptr->guid()] = character_raptr;
  }
  */
  renderer->camera_follow(characters);
  return true;
}

bool Game::init_renderer()
{
  renderer.reset(new Renderer(is_headless));
  renderer->init(config);
  renderer->camera.left = 0;
  renderer->camera.right = 2000;
  renderer->camera.top = -270;
  renderer->camera.bottom = 270;
  renderer->last_render_time_us = 0;
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

  game_path.game_root = game_root;
  return true;
}

void Game::serialize(std::vector<NetField>& list)
{
  for (int i = 0; i < entities.size(); ++i) {
    auto& entity = entities[i];
    const char* uid = reinterpret_cast<const char*>(&entity->guid()[0]);
    list.push_back(
      {"EntityMarker", NetFieldType::EntityMarker, 0, sizeof(unsigned char) * 16, uid}
    );
    entity->serialize(list);
  }
}

bool Game::deserialize(const std::vector<NetField>& fields)
{
  return true;
}

} // namespace raptr
