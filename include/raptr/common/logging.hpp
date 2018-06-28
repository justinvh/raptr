#pragma once

#include <memory>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

namespace raptr {

extern std::shared_ptr<spdlog::logger> logger;

}