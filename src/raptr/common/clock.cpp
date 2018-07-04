#include <chrono>
#include <raptr/common/clock.hpp>

namespace raptr {
namespace clock {

namespace {
auto clock_last = Time::now();
}

int64_t ticks()
{
  auto now = Time::now();
  using us = std::chrono::microseconds;
  return std::chrono::duration_cast<us>(now - clock_last).count();
}

}
}