#include <string>
#include <cxxopts.hpp>

#include <raptr/game/game.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/network/server.hpp>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

int main(int argc, char** argv)
{
  spdlog::set_level(spdlog::level::debug);
  uint32_t time_start_ms = SDL_GetTicks();

  logger->info("Hello from raptr!");

  cxxopts::Options options("raptr",
                           "You're a dinosaur without feathers struggling to understand your place in the world.");

  options.add_options()
      ("q,quiet", "Quiet the logger")
      ("g,game", "Game root path", cxxopts::value<std::string>()->default_value("../../game"));

  options.parse(argc, argv);

  if (options["quiet"].count()) {
    spdlog::set_level(spdlog::level::info);
  }

  {
    raptr::Server server("127.0.0.1:7272");
    server.fps = 20;

    std::string game_root = options["game"].as<std::string>();
    auto game = raptr::Game::create(game_root);
    server.attach(game);

    if (!server.connect()) {
      logger->error("Failed to connect to server!");
      return -1;
    }

    server.run();
  }

  uint32_t time_end_ms = SDL_GetTicks();

  uint32_t time_played = (time_end_ms - time_start_ms) / 1000.0;

  logger->info("Okay, quitting. You played for {}s. Bye Bye. Press enter to exit.", time_played);
  std::cin.get();

  return 0;
}
