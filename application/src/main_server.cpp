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

  logger->info("Hello from raptr!");

  cxxopts::Options options("raptr",
                           "You're a dinosaur without feathers struggling to understand your place in the world.");

  options.add_options()
      ("q,quiet", "Quiet the logger")
      ("g,game", "Game root path", cxxopts::value<std::string>()->default_value("../../game"));

  auto args = options.parse(argc, argv);

  if (args["quiet"].count()) {
    spdlog::set_level(spdlog::level::info);
  }

  {
    std::string game_root = args["game"].as<std::string>();
    raptr::Server server(game_root, "127.0.0.1:7272");
    server.fps = 20;

    if (!server.bind()) {
      logger->error("Failed to bind server!");
      return -1;
    }

    server.run();
  }

  logger->info("Okay, quitting. Bye Bye.");
  std::cin.get();

  return 0;
}
