#pragma once

#include <memory>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

namespace raptr {

std::shared_ptr<spdlog::logger> _get_logger(const std::string& name);

}

#define macro_enable_logger() namespace { auto logger = raptr::_get_logger(__FILE__); }