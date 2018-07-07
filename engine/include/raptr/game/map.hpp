/*!
  \file map.hpp
  This file provides the interaction and loading of meshes,
  triggers, and other sprites for a world
*/
#pragma once

#include <memory>
#include <raptr/common/filesystem.hpp>

namespace raptr {

class Map {
 public:
  std::shared_ptr<Map> load(const FileInfo& folder);
};

} // namespace raptr