#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "game.hpp"

int main(int argc, char** argv)
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

  auto game = raptr::Game::create();
  game->run();

  SDL_Quit();

  return 0;
}