#pragma once

#include <type_traits>
#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <functional>

#include <raptr/common/clock.hpp>
#include <raptr/common/rect.hpp>

#include <SDL_events.h>

namespace raptr
{
constexpr size_t MAX_SNAPSHOT_BUFFER_SIZE = 4096;
typedef std::array<unsigned char, 16> GUID;

class Trigger;
class Character;
class MeshStatic;
class MeshDynamic;

enum class NetFieldType
{
  EntityMarker,
  Character,
  MeshStatic,
  MeshDynamic,
  Trigger,
  Map,
};

enum class EngineEventType
{
  ControllerEvent,
  SpawnCharacter,
  SpawnMeshStatic,
  SpawnMeshDynamic,
  SpawnTrigger,
};

struct ControllerEvent
{
  static const EngineEventType event_type = EngineEventType::ControllerEvent;
  int32_t controller_id;
  SDL_Event sdl_event;
};

struct CharacterSpawnEvent
{
  typedef std::function<void(std::shared_ptr<Character>&)> Callback;
  static const EngineEventType event_type = EngineEventType::SpawnCharacter;
  std::string path;
  int32_t controller_id;
  GUID guid;
  Callback callback;
};

struct MeshStaticSpawnEvent
{
  typedef std::function<void(std::shared_ptr<MeshStatic>&)> Callback;
  static const EngineEventType event_type = EngineEventType::SpawnMeshStatic;
  std::string path;
  GUID guid;
  Callback callback;
};

struct TriggerSpawnEvent
{
  typedef std::function<void(std::shared_ptr<Trigger>&)> Callback;
  static const EngineEventType event_type = EngineEventType::SpawnTrigger;
  Rect rect;
  GUID guid;
  Callback callback;
};

struct MeshDynamicSpawnEvent
{
  typedef std::function<void(std::shared_ptr<MeshDynamic>&)> Callback;
  static const EngineEventType event_type = EngineEventType::SpawnMeshDynamic;
  std::string path;
  GUID guid;
  Callback callback;
};

struct EngineEvent
{
  EngineEventType type;
  int64_t time;
  void* data;

  template <class T>
  static std::shared_ptr<EngineEvent> create(T* data_)
  {
    std::shared_ptr<EngineEvent> event(new EngineEvent());
    event->data = (void*) (data_);
    event->type = T::event_type;
    event->time = clock::ticks();
    return event;
  }
};

struct NetField
{
  char* name;
  NetFieldType type;
  size_t offset;
  size_t size;
  const char* data;
  std::function<bool(const char*, const NetField&)> cmp;
  std::function<void(const NetField&)> debug;
};

struct NetPacket
{
  uint32_t seq_id;
  unsigned char guid[16];
  uint8_t num_fields;
};

struct Snapshot
{
  char buffer[MAX_SNAPSHOT_BUFFER_SIZE];
  std::vector<std::string> what_changed;
};

/*!
  This macro is to help faciliate the construction of a NetField
  object, since it is an object that relies on introspection of
  some sort, but the language lacks that.
*/
#define NetFieldMacro(klass, field)  \
    {(#klass "::" #field), \
     NetFieldType::##klass,  \
     reinterpret_cast<size_t>(&(((klass*)0)->##field)), \
     sizeof(this->##field),  \
     reinterpret_cast<const char*>(&this->##field), \
    }

class Serializable
{
public:
  virtual ~Serializable()
  {
  };
  virtual void serialize(std::vector<NetField>& list) = 0;
  virtual bool deserialize(const std::vector<NetField>& values) = 0;
};

class Network
{
public:
  static void serialize();
  std::vector<std::shared_ptr<Serializable>> snapshotters;
};
} // namespace raptr
