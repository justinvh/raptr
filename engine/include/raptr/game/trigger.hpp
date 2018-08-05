#pragma once

#include <memory>
#include <functional>

#include <raptr/game/entity.hpp>

namespace raptr {

class Trigger;

typedef std::function<void(std::shared_ptr<Character>&, Trigger*)> TriggerCallback;

/*!
 A trigger is a simple object in the world that does not cause collision, but can intersect with other entities.
 Upon activiation of the trigger, a callback is called.
*/
class Trigger : public Entity {
public:
  Trigger();
  ~Trigger() = default;
  Trigger(const Trigger&) = default;
  Trigger(Trigger&&) = default;
  Trigger& operator=(const Trigger&) = default;
  Trigger& operator=(Trigger&&) = default;

public:
  typedef TriggerSpawnEvent SpawnEvent;

  static void setup_lua_context(sol::state& state);

  static std::shared_ptr<Trigger> from_params(Rect area);

  /*!
  Returns the bounding box for this static mesh based on its sprite
  \return An rectangle containing the static mesh
  */
  Rect bbox() const override;

  /*!
  The static mesh just needs to render. It is a simple function.
  \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  void think(std::shared_ptr<Game>& game) override;

  bool intersects(const Entity* other) const override;

  bool intersects(const Rect& bbox) const override;

  bool intersects(const Entity* other, const Rect& bbox) const override;

  void serialize(std::vector<NetField>& list) override;

  bool deserialize(const std::vector<NetField>& fields) override;

  /*!
  */
  void render(Renderer* renderer) override;
public:
  std::map<std::array<unsigned char, 16>, std::shared_ptr<Character>> tracking;
  TriggerCallback on_enter;
  TriggerCallback on_exit;
  Rect shape;

public:
  bool debug;
};

} // raptr