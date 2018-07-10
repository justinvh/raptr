#include <thread>
#include <string>

#include <raptr/game/game.hpp>
#include <raptr/network/server.hpp>
#include <raptr/common/clock.hpp>

#include <raptr/common/logging.hpp>

namespace { auto logger = raptr::_get_logger(__FILE__); };

namespace raptr {

Server::Server(const fs::path& game_root,
               const std::string& server_addr_)
{
  seq_counter = 0;
  is_loopback = false;
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

  game = Game::create_headless(game_root);
}

Server::Server(const std::string& server_addr_)
{
  seq_counter = 0;
  if (server_addr_ == "loopback") {
    is_loopback = true;
    return;
  }

  is_loopback = false;
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
  if (is_loopback) {
    return;
  }

  SDLNet_UDP_Close(sock);
  SDLNet_Quit();
}

void Server::attach(std::shared_ptr<Game> game_)
{
  game = game_;
}

bool Server::connect()
{
  if (is_loopback) {
    return true;
  }

  xg::Guid guid;
  client_guid = guid.bytes();

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

void Server::send_engine_events()
{
}

void Server::run()
{
  int64_t sync_rate = static_cast<int64_t>((1.0 / fps) * 1e6);

  while (true) {
    auto current_time_us = clock::ticks();

    if (!is_client && sock && (current_time_us - frame_last_time) >= sync_rate) {
      this->update_game_state();
      frame_last_time = clock::ticks();
    }

    game->gather_engine_events();
    if (is_client) {
      this->send_engine_events();
    }
    game->process_engine_events();
  }
}

size_t Server::build_packet(size_t out_byte_off,
                            const NetField& entity_marker,
                            const std::vector<NetField>& fields,
                            size_t& field_offset)
{
  ++field_offset;
  size_t idx = 0;
  uint8_t* p = &out->data[out_byte_off];

  // Copy over the entity marker guid so the deserializer knows
  NetPacket np;
  np.seq_id = ++seq_counter;
  memcpy(np.guid, entity_marker.data, entity_marker.size);
  np.num_fields = 0;

  GUID guid_array;
  memcpy(&guid_array[0], np.guid, sizeof(unsigned char) * 16);
  auto& prev_snapshot = prev_snapshots[guid_array];
  std::shared_ptr<Snapshot> next_snapshot(new Snapshot());

  if (prev_snapshot) {
    memcpy(&next_snapshot->buffer[0], &prev_snapshot->buffer[0], MAX_SNAPSHOT_BUFFER_SIZE);
  }
  next_snapshot->what_changed.clear();

  // Start ahead and we'll come back to populate the rest later
  idx += sizeof(np);

  // Copy over field data until a marker is met
  for (; field_offset < fields.size(); ++field_offset) {
    auto& field = fields[field_offset];
    if (field.type == NetFieldType::EntityMarker) {
      --field_offset;
      break;
    }

    if ((field.offset + field.size) >= MAX_SNAPSHOT_BUFFER_SIZE) {
      throw "Class is too large for a snapshot. Why...";
    }

    // No previous data, so off we go
    if (prev_snapshot == nullptr) {
      memcpy(p + idx, field.data, field.size);
      memcpy(&next_snapshot->buffer[field.offset], field.data, field.size);
      next_snapshot->what_changed.push_back(field.name);
      idx += field.size;
      np.num_fields++;
      continue;
    }

    // Compare the previous buffer with the current
    /*
    if (field.cmp(&prev_snapshot->buffer[field.offset], field)) {
      continue;
    }
    */
    if (0 == memcmp(&prev_snapshot->buffer[field.offset], field.data, field.size)) {
      continue;
    }

    // Data does not match, so now send that message
    memcpy(p + idx, field.data, field.size);
    memcpy(&next_snapshot->buffer[field.offset], field.data, field.size);
    next_snapshot->what_changed.push_back(field.name);
    idx += field.size;
    np.num_fields++;
  }

  if (np.num_fields == 0) {
    return 0;
  }

  prev_snapshot.swap(next_snapshot);

  // Copy over the packet header
  memcpy(p, &np, sizeof(np));

  // Add the snapshot to the snapshot index
  /*
  snapshots[guid][next_snapshot_idx] = next_snapshot;
  snapshot_index[guid] = next_snapshot_idx;
  */

  // Return how many bytes we've gone
  return idx;
}

void Server::unwrap_packet()
{
}

void Server::update_game_state()
{
  if (is_client) {

    std::vector<NetField> fields;
    game->serialize(fields);

    size_t allocation_size = 0;
    size_t offset = 0;
    size_t doff = 1;
    for (size_t i = 0; i < fields.size(); ++i) {
      auto& field = fields[i];
      allocation_size += field.size;
      if (field.type == NetFieldType::EntityMarker) {
        doff += this->build_packet(doff, field, fields, i);
      }
    }

    if (doff > 1) {
      out->data[0] = 0xAA;
      out->len = doff;
      out->maxlen = doff;
      SDLNet_UDP_Send(sock, 0, out.get());
    }

  } else {
    SDLNet_UDP_Recv(sock, in.get());
    if (in->data[0] == 0xAA) {
      logger->info("Received a {} sized packet", in->len);
      this->unwrap_packet();
      in->data[0] = 0xFF;
    }
  }
}

}