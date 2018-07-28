#include <raptr/game/game.hpp>

#include <sstream>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include <functional>

#include <raptr/common/logging.hpp>
#include <raptr/config.hpp>
#include <raptr/game/character.hpp>
#include <raptr/game/game.hpp>
#include <raptr/game/trigger.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/renderer/parallax.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/game/actor.hpp>
#include <raptr/sound/sound.hpp>

namespace {
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr {

void Game::lua_trigger_wrapper(sol::state& view, sol::table trigger_params, 
                               sol::protected_function lua_on_init,
                               sol::protected_function lua_on_enter, 
                               sol::protected_function lua_on_exit)
{
  if (trigger_params == sol::nil) {
    logger->error("spawn_trigger requires 3 arguments");
  }

  const Rect trigger_rect = {
    trigger_params[1],
    trigger_params[2],
    trigger_params[3],
    trigger_params[4]
  };

  auto g = xg::newGuid();
  auto trigger_id = g.str();

  // This is basically a hack to get around the garbage collector
  // Since Lua will keep track of its shit.
  auto table = view.create_named_table(trigger_id);

  table["on_init"] = lua_on_init;
  table["on_enter"] = lua_on_enter;
  table["on_exit"] = lua_on_exit;

  this->spawn_trigger(trigger_rect, [&, trigger_id](auto& trigger)
  {
    trigger->on_enter = [&, trigger_id](std::shared_ptr<Character>& character, Trigger* trigger_l)
    {
      const auto func = view[trigger_id];
      auto result = func["on_enter"](character, trigger_l->shared_from_this());
    };

    trigger->on_exit = [&, trigger_id](std::shared_ptr<Character>& character, Trigger* trigger_l)
    {
      const auto func = view[trigger_id];
      auto result = func["on_exit"](character, trigger_l->shared_from_this());
    };

    const auto init_func = view[trigger_id];
    init_func["on_init"](trigger->shared_from_this());
  });
}

void Game::setup_lua_context(sol::state& state)
{
  using namespace std::placeholders;

  Controller::setup_lua_context(state);
  Entity::setup_lua_context(state);
  Actor::setup_lua_context(state);
  Character::setup_lua_context(state);
  Trigger::setup_lua_context(state);
  Renderer::setup_lua_context(state);

  state.new_usertype<Game>("Game",
    "controllers", &Game::controllers_active,
    "get_actor", &Game::get_entity<Actor>,
    "get_entity", &Game::get_entity<Entity>,
    "get_character", &Game::get_entity<Character>,
    "remove_entity_by_key", &Game::remove_entity_by_key,
    "remove_entity", &Game::remove_entity,
    "play_sound", [&](Game& game, std::string path) -> bool
    {
      const auto sound_path = game.game_path.from_root(path);
      if (!fs::exists(sound_path.file_path)) {
        logger->warn("Sound {} does not exist", sound_path);
        return false;
      }

      play_sound(sound_path);
      return true;
    },

    "spawn_trigger", [&](Game& game,
                         sol::table trigger_params, 
                         sol::protected_function lua_on_init, 
                         sol::protected_function lua_on_enter, 
                         sol::protected_function lua_on_exit) -> void
    {
      game.lua_trigger_wrapper(state, trigger_params, lua_on_init, lua_on_enter, lua_on_exit);
    },

    "renderer", &Game::renderer,
    "characters", [&](Game& game, int32_t n) -> std::shared_ptr<Character>
    {
      if ((n - 1) >= game.characters.size()) {
        return std::shared_ptr<Character>();
      }

      return game.characters[n - 1];
    }
  );

  state["__filename__"] = "REPL";

  state["quit"] = [&]() { this->shutdown = true; };

  state["help"] = [](const sol::this_state s, const sol::this_environment env)
  {
    sol::state_view sv(s);
    std::stringstream ss;

    ss << "You are in the Raptr REPL. There is one defined global: game\n"
      << "You can do things like, toggle fullscreen: \n\n"
      << "\t> game.renderer:toggle_fullscreen()\n\n"
      << "Or spawn a trigger: \n\n"
      << "\t> game:spawn_trigger({0, 0, 100, 100}, on_enter, on_exit)"
      << "\n";

      const auto filename = sv.get<std::string>("__filename__");
      auto lua_logger = raptr::_get_logger_lua(filename);
      lua_logger->debug(ss.str());
  };

  state["game"] = this->shared_from_this();
  state["dprintf"] = [&](const sol::this_state s, const sol::variadic_args& va)
  {
    static int32_t counter = 0;
    sol::state_view sv(s);
    lua_Debug ar;
    lua_getstack(s.L, 1, &ar);
    lua_getinfo(s.L, "nSl", &ar);
    const int line = ar.currentline;

    const auto filename = sv.get<std::string>("__filename__");
    auto lua_logger = raptr::_get_logger_lua(filename);

    std::stringstream ss;
    for (const auto& a : va) {
      ss << a.get<std::string>();
    }

    lua_logger->debug("{} [L{}] {}", counter, line, ss.str());
    ++counter;
  };
}

} // namespace raptr
