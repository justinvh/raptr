#include <functional>

#include "sprite.hpp"
#include "character.hpp"
#include "game.hpp"
#include "controller.hpp"

namespace raptr {

void Character::crouch()
{
  sprite->set_animation("Crouch", true);
}

void Character::full_hop()
{
}

void Character::run(double dx, double dy)
{
}

void Character::short_hop()
{
}



void Character::think(std::shared_ptr<Game> game)
{
  uint32_t think_delta_ms = SDL_GetTicks() - last_think_time;
  double units_moved = walk_ups * think_delta_ms / 1000.0;
  bool moved_x = true;
  bool moved_y = true;

  if (nx > 0) {
    sprite->x += units_moved;
  } else if (nx < 0) {
    sprite->x -= units_moved;
  } else {
    moved_x = false;
  }

  if (ny > 0) {
    sprite->y += units_moved;
  } else if (ny < 0) {
    sprite->y -= units_moved;
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
  nx = dx;
  ny = dy;
  sprite->set_animation("Walk Forward");
}

void Character::walk_right()
{
  curr_ups = walk_ups;
  this->walk(walk_ups, 0);
}

void Character::walk_left()
{
  curr_ups = walk_ups;
  this->walk(-walk_ups, 0);
}

void Character::walk_down()
{
  curr_ups = walk_ups;
  this->walk(0, walk_ups);
}

void Character::walk_up()
{
  curr_ups = walk_ups;
  this->walk(0, -walk_ups);
}

void Character::stop()
{
  nx = 0;
  ny = 0;
}

bool Character::on_right_joy(int32_t angle)
{
  if (angle < -4000) {
    this->walk_left();
  } else if (angle > 4000) {
    this->walk_right();
  } else {
    this->stop();
  }

  return true;
}

void Character::attach_controller(std::shared_ptr<Controller> controller_)
{
  using namespace std::placeholders;

  controller = controller_;
  controller->on_right_joy(std::bind(&Character::on_right_joy, this, _1));
}

}