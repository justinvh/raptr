#pragma once

#include <cstdint>

namespace raptr {

class Game;
class Sprite;
class Controller;

class Character {
public:
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
  virtual bool on_right_joy(int32_t angle);

public:
  std::shared_ptr<Sprite> sprite;
  std::shared_ptr<Controller> controller;
  double nx, ny;
  double walk_ups; // units-per-s
  double run_ups; // units-per-s
  double curr_ups;
  uint32_t last_think_time;
};

}
