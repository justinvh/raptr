#pragma once

#include <cstdint>

namespace raptr {

typedef std::chrono::high_resolution_clock Time;
typedef decltype(Time::now) TimePoint;

namespace clock {

int64_t ticks();

} // namespace clock
} // namespace raptr