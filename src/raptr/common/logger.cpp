#include <raptr/common/logging.hpp>
#include <experimental/filesystem>

namespace raptr {

std::shared_ptr<spdlog::logger> _get_logger(const std::string& name)
{
  std::experimental::filesystem::path path(name);
  auto filename = path.filename();
  return spdlog::stdout_color_mt(filename.string());
}

} // namespace raptr