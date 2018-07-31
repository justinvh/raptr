#include <SDL_joystick.h>

#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

#include <SDL_mixer.h>

#include <raptr/common/filesystem.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/config.hpp>
#include <raptr/game/character.hpp>
#include <raptr/game/game.hpp>
#include <raptr/game/trigger.hpp>
#include <raptr/game/map.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/renderer/parallax.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/game/actor.hpp>
#include <raptr/sound/sound.hpp>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr
{
std::shared_ptr<Game> Game::create(const fs::path& game_root)
{
  const auto full_path = fs::absolute(game_root);
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
  auto const full_path = fs::absolute(game_root);
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
  //SDL_Quit();
}

bool Game::poll_events()
{
  SDL_Event e;
  if (!SDL_PollEvent(&e)) {
    return false;
  }

  if (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
    clock::toggle();
  } else if (e.type == SDL_CONTROLLERAXISMOTION || e.type == SDL_CONTROLLERBUTTONDOWN || e.type ==
    SDL_CONTROLLERBUTTONUP ||
    e.type == SDL_JOYAXISMOTION || e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP) {
    const int32_t controller_id = e.jdevice.which;
    auto controller_event = new ControllerEvent();
    controller_event->controller_id = controller_id;
    controller_event->sdl_event = e;
    this->add_event(controller_event);
  } else if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F1) {
    renderer->toggle_fullscreen();
  } else if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F2) {
    auto controller = controllers.begin();
    ++controller;
    auto x = characters[0]->pos_.x;
    this->spawn_character("characters/raptr.toml", [&](auto& character)
    {
      character->attach_controller(controller->second);
      character->pos_.x = x;
      renderer->camera_follow(character);
    });
  } else if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F3) {
    renderer->scale(static_cast<float>(renderer->current_ratio / 2.0f));
  } else if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F4) {
    renderer->scale(1.0f);
  } else if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F5) {
    clock::toggle();
  } else if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F6) {
    const auto us = static_cast<int64_t>(1.0 / renderer->fps * 1e6);
    logger->debug("Stepping by {}ms", us / 1e3);
    clock::start();
    std::this_thread::sleep_for(std::chrono::microseconds(us));
    clock::stop();
  } else if (e.type == SDL_WINDOWEVENT) {
    if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
      this->shutdown = true;
    }
  }
  return true;
}

bool Game::gather_engine_events()
{
  if (!is_headless) {
    this->poll_events();
  }

  return true;
}

void Game::handle_controller_event(const ControllerEvent& controller_event)
{
  const auto& e = controller_event.sdl_event;
  const auto controller_id = controller_event.controller_id;

  // Is this a new character?
  if (e.type == SDL_CONTROLLERBUTTONDOWN &&
    controller_to_character.find(controller_id) == controller_to_character.end()) {
    controllers_active.push_back(controllers[controller_id]);
    this->spawn_player(controller_id);
    return;
  }

  if (e.type == SDL_CONTROLLERAXISMOTION || e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP) {
    controllers[controller_id]->process_event(e);
  } else if (e.type == SDL_JOYAXISMOTION || e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP) {
    auto& controller = controllers[controller_id];
    if (!controller->is_gamepad()) {
      controller->process_event(e);
    }
  }
}

void Game::handle_load_map_event(const LoadMapEvent& event)
{
  if (map) {
    const auto is_observed = std::find(renderer->observing.begin(),
      renderer->observing.end(),
      map);

    if (is_observed != renderer->observing.end()) {
      renderer->observing.erase(is_observed);
    }
  }

  map = Map::load(game_path.from_root(fs::path("maps") / event.name));
  if (!map) {
    logger->error("{} is not a valid map", event.name);
    return;
  }

  map->name = event.name;
  renderer->add_observable(map);
  if (event.callback) {
    event.callback(map);
  }
}

void Game::handle_character_spawn_event(const CharacterSpawnEvent& event)
{
  auto character = Character::from_toml(game_path.from_root(event.path));
  character->guid_ = event.guid;
  character->pos_.x = 0;
  character->pos_.y = 0;

  if (character->is_scripted) {
    this->setup_lua_context(character->lua);
    character->lua["instance"] = character;
    character->lua["__filename__"] = character->lua_script_fileinfo.file_relative.string();
    character->lua.safe_script(character->lua_script);
  }

  this->spawn_now(character);
  event.callback(character);
  if (character->is_scripted) {
    character->lua["init"]();
  }

  characters.push_back(character);
}

void Game::handle_actor_spawn_event(const ActorSpawnEvent& event)
{
  auto actor = Actor::from_toml(game_path.from_root(event.path));
  actor->guid_ = event.guid;
  actor->pos_.x = 0;
  actor->pos_.y = 0;
  if (actor->is_scripted) {
    this->setup_lua_context(actor->lua);
    actor->lua["instance"] = actor;
    actor->lua["__filename__"] = actor->lua_script_fileinfo.file_relative.string();
    actor->lua.safe_script(actor->lua_script);
  }

  this->spawn_now(actor);
  event.callback(actor);
  if (actor->is_scripted) {
    actor->lua["init"]();
  }
}

void Game::handle_trigger_spawn_event(const TriggerSpawnEvent& event)
{
  auto trigger = Trigger::from_params(event.rect);
  trigger->guid_ = event.guid;
  this->spawn_now(trigger);
  event.callback(trigger);
}

void Game::dispatch_event(const std::shared_ptr<EngineEvent>& event)
{
  switch (event->type) {
    case EngineEventType::LoadMap: {
      const auto map_event = reinterpret_cast<LoadMapEvent*>(event->data);
      this->handle_load_map_event(*map_event);
      delete map_event;
      break;
    }
    case EngineEventType::ControllerEvent: {
      const auto controller_event = reinterpret_cast<ControllerEvent*>(event->data);
      this->handle_controller_event(*controller_event);
      delete controller_event;
      break;
    }
    case EngineEventType::SpawnCharacter: {
      const auto character_event = reinterpret_cast<CharacterSpawnEvent*>(event->data);
      this->handle_character_spawn_event(*character_event);
      delete character_event;
      break;
    }
    case EngineEventType::SpawnActor: {
      const auto staticmesh_event = reinterpret_cast<ActorSpawnEvent*>(event->data);
      this->handle_actor_spawn_event(*staticmesh_event);
      delete staticmesh_event;
      break;
    }
    case EngineEventType::SpawnTrigger: {
      const auto trigger_event = reinterpret_cast<TriggerSpawnEvent*>(event->data);
      this->handle_trigger_spawn_event(*trigger_event);
      delete trigger_event;
      break;
    }
    default: {
      logger->info("Unhandled event type: {}", static_cast<int32_t>(event->type));
    }
  }
}

void Game::load_map(const std::string& map_name, LoadMapEvent::Callback callback)
{
  auto event = new LoadMapEvent();
  event->name = map_name;
  event->callback = callback;
  this->add_event<LoadMapEvent>(event);
}

bool Game::process_engine_events()
{
  const auto current_time_us = clock::ticks();

  frame_delta_us = current_time_us - frame_last_time;

  frame_last_time = clock::ticks();

  auto& current_events = this->engine_events_buffers[this->engine_event_index];
  this->engine_event_index = (this->engine_event_index + 1) % 2;
  for (auto& engine_event : current_events) {
    this->dispatch_event(engine_event);
  }

  auto this_ptr = this->shared_from_this();
  for (auto& entity : entities) {
    entity->think(this_ptr);

    const Point& old_point = last_known_entity_pos[entity];
    Point new_point = entity->position_abs();

    if (std::fabs(old_point.x - new_point.x) > 0.5 ||
      std::fabs(old_point.y - new_point.y) > 0.5) {
      for (auto& b : last_known_entity_bounds[entity]) {
        rtree.Remove(b.min, b.max, entity.get());
      }
      last_known_entity_bounds[entity] = entity->bounds();
      for (auto& b : entity->bounds()) {
        rtree.Insert(b.min, b.max, entity.get());
      }
      last_known_entity_pos[entity] = entity->position_abs();
    }

    if (new_point.y < -100) {
      entity->position_rel().y = 500;
    }
  }

  current_events.clear();

  if (!use_threaded_renderer) {
    renderer->run_frame();
  }

  return true;
}

bool Game::run()
{
  while (!shutdown) {
    if (!this->gather_engine_events()) {
      return false;
    }
    this->process_engine_events();
  }
  return true;
}

bool Game::intersect_world(Entity* entity, const Rect& bbox)
{
  return map->intersects(entity, bbox);
}

bool Game::intersect_anything(Entity* entity, const Rect& bbox)
{
  if (this->intersect_world(entity, bbox)) {
    return true;
  }
  const auto found = this->intersect_entity(entity, bbox);
  return !!found;
}

std::vector<std::shared_ptr<Entity>> Game::intersect_entities(
  Entity* entity, const Rect& bbox, IntersectEntityFilter post_filter, size_t limit)
{
  double min_bounds[2] = {bbox.x, bbox.y};
  double max_bounds[2] = {bbox.x + bbox.w, bbox.y + bbox.h};

  struct ConditionMet
  {
    Entity* check;
    Rect bbox;
    size_t limit;
    std::vector<Entity*> found;
    bool intersected;
    IntersectEntityFilter post_filter;
  } condition_met;

  condition_met.check = entity;
  condition_met.intersected = false;
  condition_met.bbox = bbox;
  condition_met.post_filter = post_filter;
  condition_met.limit = limit;

  rtree.Search(min_bounds, max_bounds, [](Entity* found, void* context) -> bool
  {
    const auto condition = reinterpret_cast<ConditionMet*>(context);
    Entity* self = condition->check;
    if (self->guid() == found->guid()) {
      return true;
    }

    if (!found->collidable) {
      return true;
    }

    bool has_intersection = false;
    if (self->do_pixel_collision_test && found->do_pixel_collision_test) {
      has_intersection = self->intersects(found, condition->bbox);
    } else {
      has_intersection = self->intersects(condition->bbox);
    }

    if (has_intersection && condition->post_filter(found)) {
      condition->intersected = true;
      condition->found.push_back(found);

      if (condition->limit > 0 && condition->found.size() >= condition->limit) {
        return false;
      }
    }

    return true;
  }, reinterpret_cast<void*>(&condition_met));

  if (condition_met.intersected) {
    std::vector<std::shared_ptr<Entity>> entities_found;

    for (auto& found : condition_met.found) {
      entities_found.push_back(entity_lut[found->guid()]);
    }

    return entities_found;
  }

  return {};
}

std::shared_ptr<Entity> Game::intersect_entity(Entity* entity, const Rect& bbox, IntersectEntityFilter post_filter)
{
  auto found = this->intersect_entities(entity, bbox, post_filter, 1);
  if (found.empty()) {
    return nullptr;
  }

  return found[0];
}

std::vector<std::shared_ptr<Character>> Game::intersect_characters(
  Entity* entity, const Rect& bbox, IntersectCharacterFilter post_filter, size_t limit)
{
  IntersectEntityFilter combined_filter = [&](const Entity* entity) -> bool
  {
    auto character = dynamic_cast<const Character*>(entity);
    if (!character) {
      return false;
    }

    return post_filter(character);
  };

  std::vector<std::shared_ptr<Character>> characters;
  for (auto& found : this->intersect_entities(entity, bbox, combined_filter, limit)) {
    characters.push_back(std::dynamic_pointer_cast<Character>(found));
  }

  return characters;
}

std::shared_ptr<Character> Game::intersect_character(
  Entity* entity, const Rect& bbox, IntersectCharacterFilter post_filter)
{
  auto found = this->intersect_characters(entity, bbox, post_filter, 1);
  if (found.empty()) {
    return nullptr;
  }

  return found[0];
}

void Game::spawn_player(int32_t controller_id, CharacterSpawnEvent::Callback callback)
{
  ///this->spawn_character("characters/raptr.toml", [&, controller_id, callback](auto& character)
  this->spawn_character("characters/raptr.toml", [&, controller_id, callback](auto& character)
  {
    character->position_rel().y = 300;
    character->position_rel().x = 50;
    character->flashlight = true;
    character->attach_controller(controllers[controller_id]);
    character->gravity_ps2 = gravity_ps2;
    renderer->camera_follow(character);
    controller_to_character[controller_id].push_back(character);
    callback(character);
  });
}

/*!
Spawn an entity to the world
*/
void Game::spawn_actor(const std::string& path, ActorSpawnEvent::Callback callback)
{
  auto event = new ActorSpawnEvent();
  auto g = xg::newGuid();
  event->guid = g.bytes();
  event->path = path;
  event->callback = callback;
  this->add_event<ActorSpawnEvent>(event);
}

/*!
Spawn an entity to the world
*/
void Game::spawn_character(const std::string& path, CharacterSpawnEvent::Callback callback)
{
  auto event = new CharacterSpawnEvent();
  auto g = xg::newGuid();
  event->guid = g.bytes();
  event->path = path;
  event->callback = callback;
  this->add_event<CharacterSpawnEvent>(event);
}

/*!
Spawn a trigger to the world
*/
void Game::spawn_trigger(const Rect& rect, TriggerSpawnEvent::Callback callback)
{
  auto event = new TriggerSpawnEvent();
  auto g = xg::newGuid();
  event->guid = g.bytes();
  event->rect = rect;
  event->callback = callback;
  this->add_event<TriggerSpawnEvent>(event);
}

bool Game::init()
{
  shutdown = false;
  use_threaded_renderer = false;

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

  if (!this->init_filesystem()) {
    logger->error("Failed to initialize filesystem. "
                  "Are you sure {} is the game path?", game_root);
    shutdown = true;
    return false;
  }

  config.reset(new Config());
  gravity_ps2 = -9.8 * meters_to_pixels;


  if (!this->init_controllers()) {
    logger->error("Failed to initialize controllers");
    shutdown = true;
    return false;
  }

  if (!this->init_sound()) {
    logger->error("Failed to initialize sound");
    shutdown = true;
    return false;
  }

  if (!this->init_renderer()) {
    logger->error("Failed to initialize renderer");
    shutdown = true;
    return false;
  }

  if (!this->init_lua()) {
    logger->error("Failed to initialize lua");
    shutdown = true;
    return false;
  }

  if (!this->init_demo()) {
    logger->error("Failed to initialize the demo");
    shutdown = true;
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
          logger->error(
            "At this point, we fear for the player. Not for his health, but his disorganization.");
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
    const auto controller = Controller::open(game_path, i);
    controllers[controller->id()] = controller;
  }

  return !controllers.empty();
}

void Game::spawn_now(const std::shared_ptr<Entity>& entity)
{
  auto pos = entity->position_abs();

  const auto& bounds = entity->bounds();
  last_known_entity_pos[entity] = entity->position_abs();
  last_known_entity_bounds[entity] = bounds;

  for (const auto& b : bounds) {
    rtree.Insert(b.min, b.max, entity.get());
  }

  renderer->add_observable(entity);
  entities.push_back(entity);
  entity_lut[entity->guid()] = entity;
}

bool Game::init_demo()
{
  // background test
  /*
  auto background = Parallax::from_toml(game_path.from_root("background/nightsky.toml"));
  if (background) {
    renderer->add_background(background);
  }

  auto foreground = Parallax::from_toml(game_path.from_root("foreground/nightsky.toml"));
  if (foreground) {
    renderer->add_foreground(foreground);
  }

  this->spawn_trigger({100, 0, 256, 512}, [&](auto& trigger)
  {
    trigger->on_enter = [&](std::shared_ptr<Character>& character, Trigger* trigger)
    {
      character->gravity_ps2 = -gravity_ps2;
    };

    trigger->on_exit = [&](std::shared_ptr<Character>& character, Trigger* trigger)
    {
      character->gravity_ps2 = gravity_ps2;
    };
  });

  this->spawn_trigger({500, 10, 64, 64}, [](auto& trigger)
  {
    trigger->on_enter = [](std::shared_ptr<Character>& character, Trigger* trigger)
    {
      character->velocity_rel().y += 25 * kmh_to_ps;
    };
  });

  this->spawn_actor("actors/demo/demo.toml", [](auto& mesh)
  {
    mesh->position_rel().y = 25;
  });

  this->spawn_character("actors/mad-block/mad-block.toml", [&](auto& mesh)
  {
    (*this)["mad-block"] = mesh;
    mesh->position_rel().y = 200;
    mesh->position_rel().x = 20;
  });

  map = Map::load(game_path.from_root("maps/e1m1"));
  if (!map) {
    logger->error("Failed to load map");
    return false;
  }
  renderer->add_observable(map);
  */

  this->load_map("e1m1");
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
  renderer->game_root = game_path;

  if (use_threaded_renderer) {
    renderer_thread = std::thread([&]()
    {
      while (!shutdown) {
        renderer->run_frame();
      }
    });
  }

  return true;
}

bool Game::remove_entity_by_key(const std::string key)
{
  const auto entity = this->get_entity<Entity>(key);
  if (!entity) {
    return false;
  }
  return this->remove_entity(entity);
}

bool Game::remove_entity(std::shared_ptr<Entity> entity)
{
  for (auto& child : entity->children) {
    this->remove_entity(child);
  }

  entity->children.clear();

  const auto found1 = last_known_entity_pos.find(entity);
  last_known_entity_pos.erase(found1);

  for (auto& b : last_known_entity_bounds[entity]) {
    rtree.Remove(b.min, b.max, entity.get());
  }

  const auto found2 = last_known_entity_bounds.find(entity);
  last_known_entity_bounds.erase(found2);

  const auto found3 = std::find(entities.begin(), entities.end(), entity);
  entities.erase(found3);

  std::vector<std::string> keys_to_remove;
  for (auto& pair : entity_short_lut) {
    if (pair.second == entity) {
      keys_to_remove.push_back(pair.first);
    }
  }

  for (auto& k : keys_to_remove) {
    const auto found_key = entity_short_lut.find(k);
    entity_short_lut.erase(found_key);
  }

  const auto found_guid = entity_lut.find(entity->guid());
  entity_lut.erase(found_guid);

  if (entity->is_player()) {
    const auto character = std::dynamic_pointer_cast<Character>(entity);
    const auto found4 = std::find(characters.begin(), characters.end(), character);
    for (auto& pair : controller_to_character) {
      const auto found5 = std::find(pair.second.begin(), pair.second.end(), character);
      if (found5 != pair.second.end()) {
        pair.second.erase(found5);
      }
    }
  }

  if (entity->parent) {
    entity->parent->remove_child(entity);
  }

  const auto is_followed = std::find(renderer->entities_followed.begin(),
    renderer->entities_followed.end(),
    entity);

  if (is_followed != renderer->entities_followed.end()) {
    renderer->entities_followed.erase(is_followed);
  }

  const auto is_observed = std::find(renderer->observing.begin(),
    renderer->observing.end(),
    entity);

  if (is_observed != renderer->observing.end()) {
    renderer->observing.erase(is_observed);
  }

  entity.reset();
  return true;
}

bool Game::init_sound()
{
  if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
    logger->error("Failed to initialize SDL_mixer: {}", Mix_GetError());
    return false;
  }

  Mix_AllocateChannels(64);

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

bool Game::init_lua()
{
  logger->info("Initializing Lua. Lua, what does a fox say?");
  lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::io);
  auto handler = [](lua_State*, sol::protected_function_result result)
  {
    return result;
  };

  auto result = lua.script("print('Ring-ding-ding-ding-dingeringeding!')", handler);
  if (result.valid()) {
    logger->info("Good job, Lua!");
    return true;
  }

  sol::error err = result;
  logger->error("Lua failed to initialize: {}", err.what());
  return false;
}

void Game::serialize(std::vector<NetField>& list)
{
  for (size_t i = 0; i < entities.size(); ++i) {
    auto& entity = entities[i];
    const auto uid = reinterpret_cast<const char*>(&entity->guid()[0]);
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
