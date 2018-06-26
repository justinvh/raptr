#include <functional>

#include <raptr/renderer/sprite.hpp>
#include <raptr/game/character.hpp>
#include <raptr/game/game.hpp>
#include <raptr/input/controller.hpp>

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

void Character::short_hop()
{
}

void Character::think(std::shared_ptr<Game> game)
{
  uint32_t delta_ms = this->think_delta_ms();
  auto& vel = this->velocity();
  
  // External forces, like gravity
  Rect fall_check = this->want_position_y();
  fall_check.y += game->gravity * fall_time_ms / 1000.0;
  if (game->entity_can_move_to(static_cast<Entity*>(this), fall_check)) {
    vel.y += game->gravity * fall_time_ms / 1000.0;
    fall_time_ms += delta_ms;
    falling = true;
  } else {
    if (falling) {
      vel.y = 0;
    }
    falling = false;
    fall_time_ms = 0;
  }

  Rect want_x = this->want_position_x();
  Rect want_y = this->want_position_y();

  if (game->entity_can_move_to(static_cast<Entity*>(this), want_x)) {
    if (vel.x < 0) {
      sprite->flip_x = false;
    } else if (vel.x > 0) {
      sprite->flip_x = true;
    }
    sprite->x = want_x.x;
    auto& pos = this->position();
    pos.x = want_x.x;
  }

  if (game->entity_can_move_to(static_cast<Entity*>(this), want_y)) {
    sprite->y = want_y.y;
    auto& pos = this->position();
    pos.y = want_y.y;
  }

  sprite->render(game->renderer);
  this->reset_think_delta();
}

void Character::walk(float scale)
{
  auto& vel = this->velocity();
  vel.x = scale * walk_speed;
  sprite->set_animation("Walk");
}

void Character::run(float scale)
{
  auto& vel = this->velocity();
  vel.x = scale * run_speed;
  sprite->set_animation("Run");
}

void Character::stop()
{
  auto& vel = this->velocity();
  vel.x = 0.0;
  sprite->set_animation("Idle");
}

bool Character::on_right_joy(const ControllerState& state)
{
  float mag_x = std::fabs(state.x);

  if (mag_x < 0.01f) {
    this->stop();
  } else if (mag_x < 0.5f) {
    this->walk(state.x/ 0.5f);
  } else if (mag_x >= 0.5f) {
    this->run(state.x / 1.0f);
  }

  return true;
}


bool Character::on_button_down(const ControllerState& state)
{
  if (state.button == Button::a && this->velocity().y >= 0) {
    std::clog << "Jumping!\n";
    this->velocity().y -= this->jump_vel;
    sprite->set_animation("Jump");
  }
  return true;
}

void Character::attach_controller(std::shared_ptr<Controller> controller_)
{
  using namespace std::placeholders;

  controller = controller_;
  controller->on_right_joy(std::bind(&Character::on_right_joy, this, _1));
  controller->on_button_down(std::bind(&Character::on_button_down, this, _1));
}

bool Character::intersects(const Entity* other) const
{
  if (other->id() == this->id()) {
    return false;
  }

  Rect self_box = this->bbox();
  Rect other_box = other->bbox();
  Rect res_box;
  if (!SDL_IntersectRect(&self_box, &other_box, &res_box)) {
    return false;
  }

  std::clog << other->id() << " is intersecting with " << this->id() << "\n";

  return true;
}

Rect Character::bbox() const
{
  Rect box;
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = sprite->x;
  box.y = (sprite->y);
  box.w = (current_frame.w * sprite->scale);
  box.h = (current_frame.h * sprite->scale);
  return box;
}

}