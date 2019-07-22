#include <raptr/common/logging.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <experimental/filesystem>

namespace raptr
{
std::shared_ptr<spdlog::logger> _get_logger(const std::string& name)
{
  std::experimental::filesystem::path path(name);
  auto filename = path.filename();
  return spdlog::stdout_color_mt(filename.string());
}

std::shared_ptr<spdlog::logger> _get_logger_lua(const std::string& name)
{
  auto logger = spdlog::get(name);
  if (!logger) {
    return spdlog::stdout_color_mt(name);
  }
  return logger;
}

} // namespace raptr
