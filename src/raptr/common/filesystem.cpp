#include <raptr/common/filesystem.hpp>
#include <raptr/common/logging.hpp>

macro_enable_logger();

namespace raptr {

std::optional<std::ifstream> FileInfo::open(bool binary) const
{
  fs::path full_path = file_path;
  logger->debug("Attempting to read {}", file_relative);

  if (!fs::exists(full_path)) {
    logger->error("{} does not exist", full_path);
    return {};
  }

  std::ifstream istrm(full_path, binary ? std::ios::binary : std::ios::in);

  if (istrm.is_open()) {
    return istrm;
  }

  logger->error("{} could not be opened, though it exists.", full_path);
  return {};
}

FileInfo FileInfo::from_root(const fs::path& relative_path) const
{
  fs::path full_path = game_root / relative_path;

  return FileInfo {
    relative_path,
    game_root / relative_path,
    game_root / (relative_path.parent_path()),
    game_root
  };
}

} // namespace raptr