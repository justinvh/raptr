#include <string>
#include <cxxopts.hpp>
#include <raptr/game/game.hpp>

int main(int argc, char** argv)
{
  cxxopts::Options options("raptr",
    "You're a dinosaur without feathers struggling to understand your place in the world.");

  options.add_options()
    ("d,debug", "Enable debugging")
    ("g,game", "Game root path", cxxopts::value<std::string>()->default_value("../../../game"))
    ;

  options.parse(argc, argv);

  std::string game_root = options["game"].as<std::string>();

  auto game = raptr::Game::create(game_root);
  game->run();

  return 0;
}
