#pragma once

#include <cstdint>
#include <chrono>

namespace raptr {

typedef std::chrono::high_resolution_clock Time;
typedef decltype(Time::now) TimePoint;

namespace clock {

int64_t ticks();

} // namespace clock
} // namespace raptr