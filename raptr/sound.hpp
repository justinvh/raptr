#pragma once

#include <memory>

namespace raptr {

class Config;

class Sound {
public:
  Sound() = default;
  ~Sound() = default;
  Sound(const Sound&) = default;
  Sound(Sound&&) = default;
  Sound& operator=(const Sound&) = default;
  Sound& operator=(Sound&&) = default;

  bool init(std::shared_ptr<Config> config_);
  std::shared_ptr<Config> config;
};

}
