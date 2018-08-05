#include <raptr/game/game.hpp>
#include <raptr/game/character.hpp>
#include <raptr/game/trigger.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/renderer/renderer.hpp>

namespace {
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr {

Trigger::Trigger()
  : Entity()
{
  shape.h = 0;
  shape.w = 0;
  shape.x = 0;
  shape.y = 0;
  do_pixel_collision_test = false;
  pos_.x = shape.x;
  pos_.y = shape.y;
  think_rate_us = 100;
  collidable = false;
  
}

std::shared_ptr<Trigger> Trigger::from_params(const Rect& shape)
{
  std::shared_ptr<Trigger> trigger(new Trigger);
  trigger->shape = shape;
  trigger->pos_.x = shape.x;
  trigger->pos_.y = shape.y;
  return trigger;
}

void Trigger::think(std::shared_ptr<Game>& game)
{
  auto current_think_time_us = clock::ticks();
  if (current_think_time_us - last_think_time_us < think_rate_us) {
    return;
  }
  last_think_time_us = current_think_time_us;

  if (!on_enter) {
    return;
  }

  auto intersected_characters = game->intersect_characters(this, shape);
  if (intersected_characters.empty()) {
    if (on_exit) {
      for (auto& pair : tracking) {
        on_exit(pair.second, this);
      }
    }
    tracking.clear();
    return;
  }

  std::map<std::array<unsigned char, 16>, std::shared_ptr<Character>> temp_tracking;

  for (auto& character : intersected_characters) {
    bool newly_seen = true;
    const auto guid = character->guid();
    if (tracking.find(guid) != tracking.end()) {
      newly_seen = false;
    }

    temp_tracking[guid] = character;

    if (newly_seen) {
      on_enter(character, this);
    }
  }

  if (on_exit) {
    for (auto& pair : tracking) {
      if (temp_tracking.find(pair.first) == temp_tracking.end()) {
        on_exit(pair.second, this);
      }
    }
  }

  tracking = temp_tracking;
}

Rect Trigger::bbox() const
{
  Rect rect;
  rect.w = shape.w;
  rect.h = shape.h;
  rect.x = this->position_abs().x;
  rect.y = this->position_abs().y;
  return rect;
}

bool Trigger::intersects(const Entity* other) const
{
  auto self_box = this->bbox();
  auto other_box = other->bbox();
  if (SDL_HasIntersection(&self_box, &other_box)) {
    return true;
  }
  return false;
}

bool Trigger::intersects(const Entity* other, const Rect& other_bbox) const
{
  auto self_box = this->bbox();
  if (SDL_HasIntersection(&self_box, &other_bbox)) {
    return true;
  }
  return false;
}

bool Trigger::intersects(const Rect& bbox) const
{
  auto self_box = this->bbox();
  if (SDL_HasIntersection(&self_box, &bbox)) {
    return true;
  }
  return false;
}

void Trigger::render(Renderer* renderer)
{
  SDL_Rect dst;
  dst.w = static_cast<int32_t>(shape.w);
  dst.h = static_cast<int32_t>(shape.h);
  dst.x = static_cast<int32_t>(position_abs().x);
  dst.y = static_cast<int32_t>(position_abs().y); // H - (y + dst.h));

  SDL_Color color;
  color.a = 255;
  color.r = 255;
  color.g = 0;
  color.b = 0;

  renderer->add_rect(dst, color);
}

void Trigger::setup_lua_context(sol::state& state)
{
  state.new_usertype<Trigger>("Trigger",
    sol::base_classes, sol::bases<Entity>()
  );
}

void Trigger::serialize(std::vector<NetField>& list)
{
}

bool Trigger::deserialize(const std::vector<NetField>& fields)
{
  return false;
}

}