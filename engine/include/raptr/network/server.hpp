#pragma once

#include <raptr/network/snapshot.hpp>
#include <cstdint>
#include <SDL_net.h>

namespace raptr {

class Game;

class Server {
 public:
  Server(const std::string& server_addr);
  ~Server();

  bool bind();
  bool connect();

  void attach(std::shared_ptr<Game> game);
  void run();
  void update_game_state();

 public:
  //! The number of ms since the last frame
  std::shared_ptr<Game> game;

  int32_t fps;
  std::string server_addr;
  int64_t frame_delta_us;
  int64_t frame_last_time;
  bool is_client;


public:
  UDPsocket sock;
  IPaddress ip;
  std::string ip_str;
  uint16_t port;
  std::shared_ptr<UDPpacket> in, out;
};

}