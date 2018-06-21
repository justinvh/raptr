#include <functional>

#include "sprite.hpp"
#include "character.hpp"
#include "game.hpp"
#include "controller.hpp"

namespace raptr {

int32_t Character::id() const
{
  return _id;
}

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

  bool moved_x = true, moved_y = true;
  double want_x = sprite->x, want_y = sprite->y;

  if (nx > 0) {
    want_x = sprite->x + units_moved;
  } else if (nx < 0) {
    want_x = sprite->x - units_moved;
  } else {
    moved_x = false;
  }

  if (ny > 0) {
    want_y = sprite->y + units_moved;
  } else if (ny < 0) {
    want_y = sprite->y - units_moved;
  } else {
    moved_y = false;
  }

  if (!moved_x && !moved_y) {
    sprite->set_animation("Idle");
  } else {
    SDL_Rect desired;
    desired.x = want_x;
    desired.y = want_y;
    desired.w = sprite->current_animation->current_frame().w;
    desired.h = sprite->current_animation->current_frame().h;

    if (game->entity_can_move_to(static_cast<Entity*>(this), desired)) {
      sprite->x = want_x;
      sprite->y = want_y;
    }
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

bool Character::on_right_joy(int32_t axis, int32_t angle)
{
  if (axis == 0) {
    if (angle < -4000) {
      this->walk_left();
    } else if (angle > 4000) {
      this->walk_right();
    } else {
      this->stop();
    }
  } else if (axis == 1) {
    if (angle < -4000) {
      this->walk_up();
    } else if (angle > 4000) {
      this->walk_down();
    } else {
      this->stop();
    }
  }
  return true;
}

void Character::attach_controller(std::shared_ptr<Controller> controller_)
{
  using namespace std::placeholders;

  controller = controller_;
  controller->on_right_joy(std::bind(&Character::on_right_joy, this, _1, _2));
}

bool Character::intersects(const Entity* other) const
{
  if (other->id() == this->id()) {
    return false;
  }

  SDL_Rect self_box = this->bbox();
  SDL_Rect other_box = other->bbox();
  SDL_Rect res_box;
  if (!SDL_IntersectRect(&self_box, &other_box, &res_box)) {
    return false;
  }

  return true;
}

SDL_Rect Character::bbox() const
{
  SDL_Rect box;
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = static_cast<int32_t>(sprite->x);
  box.y = static_cast<int32_t>(sprite->y);
  box.w = current_frame.w;
  box.h = current_frame.h;
  return box;
}

}