#pragma once

#include <memory>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

namespace raptr {

std::shared_ptr<spdlog::logger> _get_logger(const std::string& name);

} // namespace raptr
