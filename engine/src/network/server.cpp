#include <thread>
#include <string>

#include <raptr/game/game.hpp>
#include <raptr/network/server.hpp>
#include <raptr/common/clock.hpp>

#include <raptr/common/logging.hpp>

namespace { auto logger = raptr::_get_logger(__FILE__); };

namespace raptr {

Server::Server(const std::string& server_addr_)
{
  server_addr = server_addr_;
  size_t port_offset = server_addr.find(":");
  ip_str = server_addr.substr(0, port_offset);
  std::string port_str = server_addr.substr(port_offset + 1);
  port = static_cast<uint16_t>(std::stoul(port_str));

  if (SDLNet_Init() == -1) {
    logger->error("Failed to initialize SDLNet: {}", SDL_GetError());
    throw "SDLNet could not initialize";
  }

  in.reset(SDLNet_AllocPacket(1400));
  out.reset(SDLNet_AllocPacket(1400));
}

Server::~Server()
{
  SDLNet_UDP_Close(sock);
  SDLNet_Quit();
}

void Server::attach(std::shared_ptr<Game> game_)
{
  game = game_;
}

bool Server::connect()
{
  is_client = true;
  if (SDLNet_ResolveHost(&ip, ip_str.c_str(), port) == -1) {
    logger->error("Failed to resolve host: {}", SDLNet_GetError());
    return false;
  }

  sock = SDLNet_UDP_Open(0);
  if (SDLNet_UDP_Bind(sock, 0, &ip) == -1) {
    logger->error("Failed to connect to host: {}", SDLNet_GetError());
    return false;
  }

  // Say hello!
  out->data[0] = 1 << 4;
  out->len = 1;
  SDLNet_UDP_Send(sock, 0, out.get());
  return true;
}

bool Server::bind()
{
  is_client = false;
  sock = SDLNet_UDP_Open(port);
  if (!sock) {
    logger->error("Failed to bind to port {}: {}", port, SDLNet_GetError());
    return false;
  }

  return true;
}

void Server::run()
{
  int64_t sync_rate = static_cast<int64_t>((1.0 / fps) * 1e6);

  while (true) {
    auto current_time_us = clock::ticks();

    if (sock && (current_time_us - frame_last_time) >= sync_rate) {
      this->update_game_state();
      frame_last_time = clock::ticks();
    }

    game->run_frame();
  }
}

void Server::update_game_state()
{
  if (is_client) {
  } else {
    SDLNet_UDP_Recv(sock, in.get());
    if (in->data[0] != 1 << 4) {
      in->data[0] = 0xff;
      in->len = 1;
      SDLNet_UDP_Send(sock, -1, in.get());
    } else {
      logger->info("well, hello there.");
      in->data[0] = 0xff;
    }
  }
}

}