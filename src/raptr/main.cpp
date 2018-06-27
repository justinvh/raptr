#pragma warning(disable:4838)
#pragma warning(disable:4244)

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <raptr/game/game.hpp>

int main(int argc, char** argv)
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

  auto game = raptr::Game::create();
  game->run();

  SDL_Quit();

  return 0;
}
