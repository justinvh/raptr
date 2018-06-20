#include "config.hpp"
#include "sound.hpp"

namespace raptr {

bool Sound::init(std::shared_ptr<Config> config_)
{
  config = config_;
  return true;
}

}