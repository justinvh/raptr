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

class Tileset
{
public:
  std::string name;
  std::string texture;
  bool tiled;
};

class Object
{
  std::string name;
};

class Layer
{
public:
  std::string name;
  std::string register_collision_callback_name;
  bool draw;
  bool collision;
  bool parallax;
  std::vector<Sprite> tiles;
};

class Map
{
public:
  std::shared_ptr<Map> load(const FileInfo& folder);

public:
  std::vector<Layer> layers;
};
} // namespace raptr
