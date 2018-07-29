/*!
  \file map.hpp
  This file provides the interaction and loading of meshes,
  triggers, and other sprites for a world
*/
#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

#include <raptr/common/filesystem.hpp>
#include <raptr/renderer/sprite.hpp>

namespace raptr
{

struct Layer
{
  std::string name;
  std::vector<uint32_t> data;
};

class Map
{
public:
  std::shared_ptr<Map> load(const FileInfo& folder);

public:
  std::vector<Layer> layers;
  uint32_t width, height;
};
} // namespace raptr
