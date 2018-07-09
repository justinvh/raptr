#pragma once

#include <type_traits>
#include <cstdint>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>

namespace raptr {

constexpr size_t MAX_SNAPSHOT_BUFFER_SIZE = 4096;

enum class NetFieldType {
  EntityMarker,
  Character,
  StaticMesh,
  Map
};

struct NetField {
  char* name;
  NetFieldType type;
  size_t offset;
  size_t size;
  const char* data;
  std::function<bool(const char*, const NetField&)> cmp;
  std::function<void(const NetField&)> debug;
};

struct NetPacket {
  uint32_t seq_id;
  unsigned char guid[16];
  uint8_t num_fields;
};

struct Snapshot {
  char buffer[MAX_SNAPSHOT_BUFFER_SIZE];
  std::vector<std::string> what_changed;
};

/*!
  This macro is to help faciliate the construction of a NetField
  object, since it is an object that relies on introspection of
  some sort, but the language lacks that.
*/
#define NetFieldMacro(klass, field) \
    {(#klass "::" #field), \
     NetFieldType::##klass, \
     reinterpret_cast<size_t>(&(((klass*)0)->##field)), \
     sizeof(this->##field), \
     reinterpret_cast<const char*>(&this->##field), \
    }

class Serializable {
 public:
  virtual ~Serializable() {};
  virtual void serialize(std::vector<NetField>& list) = 0;
  virtual bool deserialize(const std::vector<NetField>& values) = 0;
};

class Network {
 public:
  void serialize();
  std::vector<std::shared_ptr<Serializable>> snapshotters;
};

} // namespace raptr