#include <string>
#include <cxxopts.hpp>

#include <raptr/game/game.hpp>
#include <raptr/common/logging.hpp>

macro_enable_logger();

int main(int argc, char** argv)
{
  spdlog::set_level(spdlog::level::debug);

  logger->info("Hello from raptr!");

  cxxopts::Options options("raptr",
    "You're a dinosaur without feathers struggling to understand your place in the world.");

  options.add_options()
    ("q,quiet", "Quiet the logger")
    ("g,game", "Game root path", cxxopts::value<std::string>()->default_value("../game"))
    ;

  options.parse(argc, argv);

  if (options["quiet"].count()) {
    spdlog::set_level(spdlog::level::info);
  }

  std::string game_root = options["game"].as<std::string>();
  auto game = raptr::Game::create(game_root);
  game->run();

  return 0;
}
