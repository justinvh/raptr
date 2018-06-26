#pragma once

#include <cstdint>

#include "entity.hpp"

namespace raptr {

class Game;
class Sprite;
class Controller;
struct ControllerState;

class Character : public Entity {
public:
  virtual bool intersects(const Entity* other) const;
  virtual int32_t id() const;
  virtual Rect bbox() const;
  virtual void walk(float scale);
  virtual void run(float scale);
  virtual void crouch();
  virtual void stop();
  virtual void short_hop();
  virtual void full_hop();
  virtual void think(std::shared_ptr<Game> game);

  virtual void attach_controller(std::shared_ptr<Controller> controller);

private:
  virtual bool on_right_joy(const ControllerState& state);
  virtual bool on_button_down(const ControllerState& state);

public:
  std::shared_ptr<Sprite> sprite;
  std::shared_ptr<Controller> controller;
  int32_t _id;

  bool falling;
  uint32_t walk_speed;
  uint32_t run_speed;
  uint32_t jump_vel;
};

}
