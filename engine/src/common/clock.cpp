#include <chrono>
#include <raptr/common/clock.hpp>
#include <raptr/common/logging.hpp>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr
{
namespace clock
{
namespace
{
auto clock_last = Time::now();
auto paused_time = Time::now();
int64_t offset_us = 0;
bool paused = false;
using us = std::chrono::microseconds;
}

int64_t ticks()
{
  auto now = Time::now();
  if (paused) {
    now = paused_time;
  }
  return std::chrono::duration_cast<us>(now - clock_last - us(offset_us)).count();
}

void start()
{
  if (paused) {
    toggle();
  }
}

void stop()
{
  if (!paused) {
    toggle();
  }
}

bool toggle()
{
  paused = !paused;
  if (!paused) {
    auto now = Time::now();
    offset_us += std::chrono::duration_cast<us>(now - paused_time).count();
    logger->debug("Clock is now offset by {}us", offset_us);
  } else {
    paused_time = Time::now();
  }

  return paused;
}
}
}
