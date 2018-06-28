#include <raptr/common/logging.hpp>

namespace raptr {

std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("raptr");

} // namespace raptr