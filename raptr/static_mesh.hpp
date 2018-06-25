#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "entity.hpp"

namespace raptr {

class Sprite;
class Game;

class StaticMesh : public Entity {
public:
  virtual bool intersects(const Entity* other) const;
  virtual int32_t id() const;
  virtual SDL_Rect bbox() const;
  virtual void think(std::shared_ptr<Game> game);

public:
  std::shared_ptr<Sprite> sprite;
  int32_t _id;
};


}
