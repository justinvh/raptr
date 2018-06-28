#pragma once

#include <fstream>
#include <experimental/filesystem>

namespace raptr {
namespace fs {

using Path = std::experimental::filesystem::path;

std::ifstream open(const Path& path);

} // namespace fs
} // namespace raptr