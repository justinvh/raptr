#include <raptr/common/filesystem.hpp>
#include <raptr/common/logging.hpp>
#include <sstream>

namespace {
auto logger = raptr::_get_logger(__FILE__);
};

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

std::optional<std::string> FileInfo::read(bool binary) const
{
    auto ifs = this->open(binary);
    if (!ifs) {
        return {};
    }

    std::stringstream ss;
    ss << ifs->rdbuf();
    return ss.str();
}

FileInfo FileInfo::from_root(const fs::path& relative_path) const
{
    fs::path full_path = game_root / relative_path;

    return FileInfo {
        relative_path,
        game_root / relative_path,
        game_root / relative_path.parent_path(),
        game_root
    };
}

FileInfo FileInfo::from_current_dir(const fs::path& relative_path) const
{
    const auto is_file = fs::is_regular_file(file_path);
    const auto new_file_path = is_file ? file_path.parent_path() : file_path;
    const auto new_file_relative = is_file ? file_relative.parent_path() : file_relative;
    return {
        new_file_relative / relative_path,
        fs::absolute(new_file_path / relative_path),
        new_file_path,
        game_root
    };
}

FileInfo FileInfo::operator/(const fs::path& path) const
{
    return {
        file_relative / path,
        file_path / path,
        (file_path / path).parent_path(),
        game_root
    };
}

FileInfo& FileInfo::operator/=(const fs::path& path)
{
    file_relative /= path;
    file_path /= path;
    file_dir = file_path.parent_path();
    return *this;
}

} // namespace raptr
