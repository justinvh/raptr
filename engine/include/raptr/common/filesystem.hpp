#pragma once

#include <optional>
#include <ostream>
#include <fstream>
#include <fmt/ostream.h>
#include <experimental/filesystem>

namespace raptr
{
namespace fs = std::experimental::filesystem;

class Filesystem;

class FileInfo
{
public:
  fs::path file_relative;
  fs::path file_path;
  fs::path file_dir;
  fs::path game_root;

  FileInfo operator/ (const fs::path& path) const;
  FileInfo& operator/= (const fs::path& path);

public:
  std::optional<std::ifstream> open(bool binary = true) const;
  std::optional<std::string> read(bool binary = true) const;
  FileInfo from_root(const fs::path& relative_path) const;
  FileInfo from_current_dir(const fs::path& relative_path) const;
};

inline std::ostream& operator<<(std::ostream& os, const FileInfo& f)
{
  os << f.file_relative;
  return os;
}
} // namespace raptr
