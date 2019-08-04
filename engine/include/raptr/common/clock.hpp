#pragma once

#include <chrono>
#include <cstdint>

namespace raptr::clock {
typedef std::chrono::high_resolution_clock Time;
typedef decltype(Time::now) TimePoint;
int64_t ticks();
bool toggle();
void start();
void stop();

} // namespace raptr::clock
