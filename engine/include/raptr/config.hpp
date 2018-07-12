#pragma once

namespace raptr
{
class Config
{
public:
  Config() = default;
  ~Config() = default;
  Config(const Config&) = default;
  Config(Config&&) = default;
  Config& operator=(const Config&) = default;
  Config& operator=(Config&&) = default;
};
} // namespace raptr
