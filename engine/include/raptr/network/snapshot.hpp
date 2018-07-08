#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace raptr {

enum class NetFieldType {

};

struct NetField {
  char* name;
  int32_t offset;
  int32_t bits;
};

class Serializable {
 public:
  virtual ~Serializable() {};
  virtual std::vector<NetField> serialize() = 0;
  virtual bool deserialize(const std::vector<NetField>& values) = 0;
};

class Network {
 public:
  void serialize();
  std::vector<std::shared_ptr<Serializable>> snapshotters;
};

} // namespace raptr