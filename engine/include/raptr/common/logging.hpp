#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace raptr {
std::shared_ptr<spdlog::logger> _get_logger(const std::string& name);
std::shared_ptr<spdlog::logger> _get_logger_lua(const std::string& name);
} // namespace raptr
