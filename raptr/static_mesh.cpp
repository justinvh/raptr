#include <iostream>

#include "game.hpp"
#include "sprite.hpp"
#include "static_mesh.hpp"

namespace raptr {

int32_t StaticMesh::id() const
{
  return _id;
}

bool StaticMesh::intersects(const Entity* other) const
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

  std::clog << other->id() << " is intersecting with " << this->id() << "\n";

  return true;
}

SDL_Rect StaticMesh::bbox() const
{
  SDL_Rect box;
  auto& current_frame = sprite->current_animation->current_frame();
  box.x = static_cast<int32_t>(sprite->x);
  box.y = static_cast<int32_t>(sprite->y);
  box.w = current_frame.w * sprite->scale;
  box.h = current_frame.h * sprite->scale;
  return box;
}


void StaticMesh::think(std::shared_ptr<Game> game)
{
  sprite->render(game->renderer);
}

}