#pragma once

#include <cstdint>

namespace raptr {

class Game;
class Sprite;

class Character {
public:
  bool is_moving();
  virtual void walk(double dx, double dy);
  virtual void walk_left(double secs);
  virtual void walk_right(double secs);
  virtual void walk_up(double secs);
  virtual void walk_down(double secs);
  virtual void stop();
  virtual void run(double dx, double dy);
  virtual void short_hop();
  virtual void full_hop();
  virtual void think(std::shared_ptr<Game> game);

public:
  std::shared_ptr<Sprite> sprite;
  double nx, ny;
  double walk_ups; // units-per-s
  double run_ups; // units-per-s
  double curr_ups;
  uint32_t last_think_time;
};

}
