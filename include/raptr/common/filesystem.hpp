#pragma once

#include <optional>
#include <ostream>
#include <fstream>
#include <fmt/ostream.h>
#include <experimental/filesystem>

namespace raptr {

namespace fs = std::experimental::filesystem;

class Filesystem;

class FileInfo {
 public:
  fs::path file_relative;
  fs::path file_path;
  fs::path file_dir;
  fs::path game_root;

 public:
  std::optional<std::ifstream> open(bool binary = true) const;
  FileInfo from_root(const fs::path& relative_path) const;
};

} // namespace raptr
