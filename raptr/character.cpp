#include "sprite.hpp"
#include "character.hpp"
#include "game.hpp"

namespace raptr {

void Character::full_hop()
{
}

void Character::run(double dx, double dy)
{
}

void Character::short_hop()
{
}


bool Character::is_moving()
{
  return (sprite->x != nx || sprite->y != ny);
}

void Character::think(std::shared_ptr<Game> game)
{
  uint32_t think_delta_ms = SDL_GetTicks() - last_think_time;
  double units_moved = walk_ups * think_delta_ms / 1000.0;
  bool moved_x = true;
  bool moved_y = true;

  if (sprite->x < nx) {
    sprite->x += units_moved;
    sprite->flip_x = true;
    if (sprite->x > nx) {
      sprite->x = nx;
    }
  } else if (sprite->x > nx) {
    sprite->x -= units_moved;
    sprite->flip_x = false;
    if (sprite->x < nx) {
      sprite->x = nx;
    }
  } else {
    moved_x = false;
  }
  
  if (sprite->y < ny) {
    sprite->y += units_moved;
    if (sprite->y > ny) {
      sprite->y = ny;
    }
  } else if (sprite->y > ny) {
    sprite->y -= units_moved;
    if (sprite->y < ny) {
      sprite->y = ny;
    }
  } else {
    moved_y = false;
  }

  if (!moved_x && !moved_y) {
    sprite->set_animation("Idle");
  }

  sprite->render(game->renderer);
  last_think_time = SDL_GetTicks();
}

void Character::walk(double dx, double dy)
{
  nx = sprite->x + dx;
  ny = sprite->y + dy;
  sprite->set_animation("Walk Forward");
}

void Character::walk_right(double secs)
{
  curr_ups = walk_ups;
  this->walk(walk_ups * secs, 0);
}

void Character::walk_left(double secs)
{
  curr_ups = walk_ups;
  this->walk(-walk_ups * secs, 0);
}

void Character::walk_down(double secs)
{
  curr_ups = walk_ups;
  this->walk(0, walk_ups * secs);
}

void Character::walk_up(double secs)
{
  curr_ups = walk_ups;
  this->walk(0, -walk_ups * secs);
}

void Character::stop()
{
  nx = sprite->x;
  ny = sprite->y;
}

}