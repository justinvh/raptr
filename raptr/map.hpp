#pragma once

#include <memory>
#include <string>

namespace raptr {

class Map {
public:
  std::shared_ptr<Map> from_path(const std::string& world,
                                 const std::string& collision);
};

}
