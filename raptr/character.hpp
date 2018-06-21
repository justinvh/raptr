#pragma once

#include <cstdint>

#include "entity.hpp"

namespace raptr {

class Game;
class Sprite;
class Controller;

class Character : public Entity {
public:
  virtual bool intersects(const Entity* other) const;
  virtual int32_t id() const;
  virtual SDL_Rect bbox() const;
  virtual void walk(double dx, double dy);
  virtual void walk_left();
  virtual void walk_right();
  virtual void walk_up();
  virtual void walk_down();
  virtual void crouch();
  virtual void stop();
  virtual void run(double dx, double dy);
  virtual void short_hop();
  virtual void full_hop();
  virtual void think(std::shared_ptr<Game> game);

  virtual void attach_controller(std::shared_ptr<Controller> controller);

private:
  virtual bool on_right_joy(int32_t joystick, float angle, float magnitude, float x, float y);

public:
  std::shared_ptr<Sprite> sprite;
  std::shared_ptr<Controller> controller;
  int32_t _id;
  double nx, ny;
  double walk_ups; // units-per-s
  double run_ups; // units-per-s
  double curr_ups;
  uint32_t last_think_time;
};

}
