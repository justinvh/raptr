#include <thread>
#include <raptr/game/game.hpp>
#include <raptr/network/server.hpp>
#include <raptr/common/clock.hpp>

namespace raptr {

Server::Server(const std::string& server_addr_)
{
  server_addr = server_addr_;
}

void Server::attach(std::shared_ptr<Game> game_)
{
  game = game_;
}

void Server::run()
{
  int64_t sync_rate = (1.0 / fps) * 1e6;

  while (true) {
    auto current_time_us = clock::ticks();

    if ((current_time_us - frame_last_time) >= sync_rate) {
      this->update_game_state();
      frame_last_time = clock::ticks();
    }

    game->run_frame();
  }
}

void Server::update_game_state()
{
}

}