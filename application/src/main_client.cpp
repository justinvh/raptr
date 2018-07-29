#include <string>
#include <cxxopts.hpp>

#include <raptr/game/game.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/network/server.hpp>

#include <discord_rpc.h>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
static const char* discord_application_id = "472884672112623616";
};

void discord_init()
{
  DiscordEventHandlers handlers;
  memset(&handlers, 0, sizeof(handlers));
  Discord_Initialize(discord_application_id, &handlers, 1, nullptr);
}

void discord_update_presence()
{
  DiscordRichPresence discordPresence;
  memset(&discordPresence, 0, sizeof(discordPresence));
  discordPresence.state = "Engine Development";
  discordPresence.details = "Platforming Around";
  discordPresence.largeImageKey = "raptr-happy_png";
  discordPresence.largeImageText = "Raptr";
  discordPresence.partySize = 1;
  discordPresence.partyMax = 1;
  Discord_UpdatePresence(&discordPresence);
}

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

    const std::string game_root = options["game"].as<std::string>();
    auto game = raptr::Game::create(game_root);
    server.attach(game);

    if (!server.connect()) {
      logger->error("Failed to connect to server!");
      return -1;
    }

    discord_init();
    discord_update_presence();

    server.run();
  }

  Discord_ClearPresence();

  const auto time_end_ms = SDL_GetTicks();
  const auto time_played = static_cast<uint32_t>((time_end_ms - time_start_ms) / 1000.0);

  logger->info("Okay, quitting. You played for {}s. Bye Bye. Press enter to exit.", time_played);
  std::cin.get();

  return 0;
}
