#pragma once

#include <array>
#include <map>
#include <cstdint>

#include <SDL_net.h>

#include <raptr/network/snapshot.hpp>

namespace raptr
{
constexpr size_t MAX_SNAPSHOTS = 32;

class Game;

enum class ServerEvent
{
};


typedef std::map<GUID, std::shared_ptr<Snapshot>> EntityToSnapshot;
typedef std::map<GUID, EntityToSnapshot> ClientSnapshots;

class Server
{
public:
  Server(const fs::path& game_root,
         const std::string& server_addr);
  Server(const std::string& server_addr);
  ~Server();

  bool bind();
  bool connect();

  void attach(std::shared_ptr<Game> game);
  void run();
  void update_game_state();

  size_t build_packet(size_t out_byte_off,
                      const NetField& entity_marker,
                      const std::vector<NetField>& fields,
                      size_t& field_offset);

  static void unwrap_packet();
  static void send_engine_events();

public:
  //! The number of ms since the last frame
  std::shared_ptr<Game> game;

  int32_t fps;
  std::string server_addr;
  int64_t frame_delta_us;
  int64_t frame_last_time;
  bool is_client;
  bool is_loopback;

  EntityToSnapshot prev_snapshots;
  ClientSnapshots client_snapshots;

public:
  GUID client_guid;
  uint32_t seq_counter;
  UDPsocket sock;
  IPaddress ip;
  std::string ip_str;
  uint16_t port;
  std::shared_ptr<UDPpacket> in, out;
};
}
