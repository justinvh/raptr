#include "game.hpp"

int main(int argc, char** argv)
{
  raptr::Game game;
  game.run();

  SDL_Quit();

  return 0;
}