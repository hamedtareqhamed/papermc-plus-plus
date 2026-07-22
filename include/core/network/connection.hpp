#ifndef PAPERMC_CORE_NETWORK_CONNECTION_HPP
#define PAPERMC_CORE_NETWORK_CONNECTION_HPP

#include <memory>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <cstring>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include "core/protocol/packet.hpp"
#include "core/protocol/play_packets.hpp"
#include "core/protocol/buffer.hpp"
#include "core/protocol/encryption.hpp"

namespace papermc::core::network {

enum class PlayerLifecycleState : uint8_t {
    JOINING,
    SPAWNED_AND_READY
};

inline std::string to_hex_dump(std::span<const std::byte> bytes, std::size_t max_bytes = 64) {
    std::stringstream ss;
    std::size_t count = std::min(bytes.size(), max_bytes);
    for (std::size_t i = 0; i < count; ++i) {
        ss << fmt::format("{:02X} ", static_cast<uint8_t>(bytes[i]));
    }
    if (bytes.size() > max_bytes) {
        ss << fmt::format("... ({} total bytes)", bytes.size());
    }
    return ss.str();
}

class ServerConnection : public std::enable_shared_from_this<ServerConnection> {
public:
    explicit ServerConnection(asio::ip::tcp::socket socket, bool offline_mode = true)
        : socket_(std::move(socket)),
          offline_mode_(offline_mode),
          keep_alive_timer_(socket_.get_executor()),
          teleport_confirm_timer_(socket_.get_executor()),
          last_keep_alive_timestamp_(std::chrono::steady_clock::now()) {}

    ~ServerConnection() {
        keep_alive_timer_.cancel();
        teleport_confirm_timer_.cancel();
    }

    void start() {
        std::string remote_addr = socket_.remote_endpoint().address().to_string();
        spdlog::info("Client connected from {}", remote_addr);
        do_read_packet_length();
    }

    void close() {
        keep_alive_timer_.cancel();
        teleport_confirm_timer_.cancel();
        asio::error_code ec;
        socket_.close(ec);
    }

private:
    [[nodiscard]] std::string state_to_string() const noexcept {
        switch (state_) {
            case protocol::ProtocolState::Handshake: return "HANDSHAKE";
            case protocol::ProtocolState::Status: return "STATUS";
            case protocol::ProtocolState::Login: return "LOGIN";
            case protocol::ProtocolState::Configuration: return "CONFIGURATION";
            case protocol::ProtocolState::Play: return "PLAY";
        }
        return "UNKNOWN";
    }

    void send_packet_buf(const protocol::ByteBuf& pkt_buf) {
        auto raw_data = pkt_buf.data_span();

        protocol::ByteBuf read_buf(raw_data);
        auto pkt_id_res = read_buf.read_varint();
        int32_t pkt_id = pkt_id_res ? *pkt_id_res : -1;

        std::string state_str = state_to_string();
        spdlog::info("[Network OUT] Sent Packet ID: 0x{:02X} (Length: {} bytes) in state: {}",
                     pkt_id, raw_data.size(), state_str);

        if (state_ == protocol::ProtocolState::Play) {
            spdlog::info("[Network OUT HexDump 0x{:02X}]: {}", pkt_id, to_hex_dump(raw_data));
        }

        protocol::ByteBuf frame_buf;
        frame_buf.write_varint(static_cast<int32_t>(raw_data.size()));
        frame_buf.write_bytes(raw_data);

        auto send_span = frame_buf.data_span();
        auto out_bytes = std::make_shared<std::vector<uint8_t>>(send_span.size());
        std::memcpy(out_bytes->data(), send_span.data(), send_span.size());

        auto self(shared_from_this());
        asio::async_write(
            socket_,
            asio::buffer(*out_bytes),
            [self, out_bytes](asio::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    spdlog::warn("Failed sending packet: {}", ec.message());
                }
            }
        );
    }

    template <typename PacketType>
    void send_packet(const PacketType& packet) {
        protocol::ByteBuf pkt_buf;
        packet.serialize(pkt_buf);
        send_packet_buf(pkt_buf);
    }

    void start_keep_alive_timer() {
        auto self(shared_from_this());
        keep_alive_timer_.expires_after(std::chrono::seconds(5));
        keep_alive_timer_.async_wait([this, self](asio::error_code ec) {
            if (!ec && state_ == protocol::ProtocolState::Play) {
                int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count();
                protocol::KeepAliveClientbound26_2 keep_alive{.keep_alive_id = now_ms};
                send_packet(keep_alive);
                start_keep_alive_timer();
            }
        });
    }

    void start_teleport_confirm_timer() {
        auto self(shared_from_this());
        teleport_confirmed_ = false;
        teleport_confirm_timer_.expires_after(std::chrono::seconds(2));
        teleport_confirm_timer_.async_wait([this, self](asio::error_code ec) {
            if (!ec && !teleport_confirmed_ && state_ == protocol::ProtocolState::Play) {
                spdlog::warn("[DEBUG] Waiting for Client Confirm Teleportation packet (0x00)...");
            }
        });
    }

    void do_read_packet_length() {
        auto self(shared_from_this());
        socket_.async_read_some(
            asio::buffer(read_buffer_.data(), 5),
            [this, self](asio::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    if (ec != asio::error::eof) {
                        spdlog::error("Socket read error: {}", ec.message());
                    }
                    return;
                }

                std::span<const std::byte> span(
                    reinterpret_cast<const std::byte*>(read_buffer_.data()),
                    bytes_transferred
                );

                auto len_res = protocol::decode_varint(span);
                if (!len_res) {
                    spdlog::warn("Invalid packet length VarInt received.");
                    return;
                }

                std::size_t packet_len = static_cast<std::size_t>(len_res->value);
                std::size_t header_consumed = len_res->bytes_read;

                do_read_packet_payload(packet_len, header_consumed, bytes_transferred);
            }
        );
    }

    void do_read_packet_payload(std::size_t packet_len, std::size_t header_consumed, std::size_t bytes_transferred) {
        auto self(shared_from_this());
        packet_payload_buffer_.resize(packet_len);

        std::size_t initial_bytes = 0;
        if (bytes_transferred > header_consumed) {
            initial_bytes = bytes_transferred - header_consumed;
            std::memcpy(packet_payload_buffer_.data(), read_buffer_.data() + header_consumed, initial_bytes);
        }

        if (initial_bytes >= packet_len) {
            process_packet(packet_payload_buffer_);
            do_read_packet_length();
            return;
        }

        asio::async_read(
            socket_,
            asio::buffer(packet_payload_buffer_.data() + initial_bytes, packet_len - initial_bytes),
            [this, self](asio::error_code ec, std::size_t /*bytes_transferred*/) {
                if (!ec) {
                    process_packet(packet_payload_buffer_);
                    do_read_packet_length();
                } else {
                    spdlog::warn("Failed reading packet payload: {}", ec.message());
                }
            }
        );
    }

    void process_packet(const std::vector<uint8_t>& payload) {
        std::span<const std::byte> span(
            reinterpret_cast<const std::byte*>(payload.data()),
            payload.size()
        );
        protocol::ByteBuf buf(span);

        auto pkt_id_res = buf.read_varint();
        if (!pkt_id_res) return;

        int32_t packet_id = *pkt_id_res;
        
        std::string state_str = state_to_string();
        spdlog::info("[Network IN] Received Packet ID: 0x{:02X} (Length: {} bytes) in state: {}",
                     packet_id, payload.size(), state_str);

        // STATE 0: HANDSHAKE
        if (state_ == protocol::ProtocolState::Handshake && packet_id == 0x00) {
            auto handshake = protocol::HandshakePacket::deserialize(buf);
            if (handshake) {
                spdlog::info("Handshake received! Protocol Version: {}, Host: {}:{}, Next State: {}",
                             handshake->protocol_version, handshake->server_address, handshake->server_port, handshake->next_state);
                state_ = (handshake->next_state == 1) ? protocol::ProtocolState::Status : protocol::ProtocolState::Login;
            }
        }
        // STATE 1: STATUS
        else if (state_ == protocol::ProtocolState::Status) {
            if (packet_id == 0x00) { // Status Request
                protocol::StatusResponsePacket response{
                    .json_payload = R"({"version":{"name":"26.2","protocol":776},"players":{"max":100,"online":1},"description":{"text":"PaperMC++ 26.2 High-Performance Core Engine"}})"
                };
                send_packet(response);
            } else if (packet_id == 0x01) { // Ping Request
                auto ping = protocol::PingPacket::deserialize(buf);
                if (ping) {
                    send_packet(*ping);
                }
            }
        }
        // STATE 2: LOGIN
        else if (state_ == protocol::ProtocolState::Login) {
            if (packet_id == 0x00) { // Login Start
                auto login_start = protocol::LoginStartPacket::deserialize(buf);
                if (login_start) {
                    username_ = login_start->username;
                    spdlog::info("Login Start received for player '{}' (offline-mode: {})", username_, offline_mode_);

                    if (offline_mode_) {
                        std::array<uint8_t, 16> uuid_bytes = protocol::parse_uuid_string("00000000-0000-0000-0000-000000000001");
                        protocol::LoginSuccessPacket login_success{
                            .uuid = uuid_bytes,
                            .username = username_
                        };
                        send_packet(login_success);
                    }
                }
            } else if (packet_id == 0x03) { // Login Acknowledged (26.2 Protocol 776)
                spdlog::info("Login Acknowledged received for player '{}'. Transitioning to CONFIGURATION state...", username_);
                state_ = protocol::ProtocolState::Configuration;
                
                // 1. Send Select Known Packs Packet (Clientbound 0x0E in CONFIGURATION state for 26.2)
                protocol::SelectKnownPacksPacket known_packs;
                send_packet(known_packs);
            }
        }
        // STATE 4: CONFIGURATION
        else if (state_ == protocol::ProtocolState::Configuration) {
            if (packet_id == 0x00) { // Client Information
                spdlog::info("[Network IN] Client information received in CONFIGURATION state for player '{}'", username_);
            } else if (packet_id == 0x07) { // Serverbound Select Known Packs (0x07 in CONFIGURATION)
                spdlog::info("[Network IN] Select Known Packs response received from player '{}'. Sending Registry Data & Finish Configuration...", username_);
                
                // 2. Send Update Enabled Features (Clientbound 0x0C)
                protocol::UpdateEnabledFeaturesPacket features_pkt;
                send_packet(features_pkt);

                // 3. Send Registry Data Packets (Clientbound 0x07 for 26.2)
                protocol::RegistryDataPacket damage_type_registry{
                    .registry_id = "minecraft:damage_type",
                    .entry_ids = {
                        "minecraft:in_fire", "minecraft:on_fire", "minecraft:lava", "minecraft:in_wall",
                        "minecraft:drown", "minecraft:starve", "minecraft:cactus", "minecraft:fall",
                        "minecraft:fly_into_wall", "minecraft:out_of_world", "minecraft:generic", "minecraft:magic",
                        "minecraft:wither", "minecraft:dragon_breath", "minecraft:dry_out", "minecraft:sweet_berry_bush",
                        "minecraft:freeze", "minecraft:stalagmite", "minecraft:falling_block", "minecraft:arrow",
                        "minecraft:fireball", "minecraft:lightning_bolt", "minecraft:mob_attack", "minecraft:player_attack",
                        "minecraft:thorns", "minecraft:explosion"
                    },
                    .include_overworld_nbt = false
                };
                send_packet(damage_type_registry);

                protocol::RegistryDataPacket dim_type_registry{
                    .registry_id = "minecraft:dimension_type",
                    .entry_ids = {"minecraft:overworld"},
                    .include_overworld_nbt = true
                };
                send_packet(dim_type_registry);

                protocol::RegistryDataPacket biome_registry{
                    .registry_id = "minecraft:biome",
                    .entry_ids = {"minecraft:plains"},
                    .include_overworld_nbt = false
                };
                send_packet(biome_registry);

                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:cat_variant", .entry_ids = {"minecraft:tabby"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:cat_sound_variant", .entry_ids = {"minecraft:cat"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:chicken_variant", .entry_ids = {"minecraft:temperate"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:chicken_sound_variant", .entry_ids = {"minecraft:chicken"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:cow_variant", .entry_ids = {"minecraft:temperate"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:cow_sound_variant", .entry_ids = {"minecraft:cow"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:frog_variant", .entry_ids = {"minecraft:temperate"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:painting_variant", .entry_ids = {"minecraft:kebab"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:pig_variant", .entry_ids = {"minecraft:temperate"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:pig_sound_variant", .entry_ids = {"minecraft:pig"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:wolf_variant", .entry_ids = {"minecraft:pale"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:wolf_sound_variant", .entry_ids = {"minecraft:wolf"}, .include_overworld_nbt = false});
                send_packet(protocol::RegistryDataPacket{.registry_id = "minecraft:zombie_nautilus_variant", .entry_ids = {"minecraft:temperate"}, .include_overworld_nbt = false});

                // 4. Send Update Tags Packet (Clientbound 0x0D)
                protocol::UpdateTagsPacket tags_pkt;
                send_packet(tags_pkt);

                // 5. Send Finish Configuration Packet (Clientbound 0x03 in CONFIGURATION for 26.2)
                protocol::FinishConfigurationPacket finish_config;
                send_packet(finish_config);
            } else if (packet_id == 0x03) { // Acknowledge Finish Configuration (Serverbound 0x03 in CONFIGURATION)
                spdlog::info("Finish Configuration acknowledged by player '{}'. Transitioning to PLAY state...", username_);
                state_ = protocol::ProtocolState::Play;

                // 1. Send Login (play) Packet (Clientbound 0x31 for 26.2 Protocol 776)
                protocol::LoginPlayPacket26_2 login_play;
                send_packet(login_play);

                // 2. Send Synchronize Player Position Packet (Clientbound 0x48 for 26.2 Protocol 776)
                protocol::PlayerPositionPacket26_2 pos_pkt{
                    .teleport_id = 1,
                    .x = 0.0,
                    .y = 64.0,
                    .z = 0.0,
                    .vx = 0.0,
                    .vy = 0.0,
                    .vz = 0.0,
                    .yaw = 0.0f,
                    .pitch = 0.0f,
                    .flags = 0
                };
                send_packet(pos_pkt);
                start_teleport_confirm_timer();

                // 3. Send Game Event Packet (Clientbound 0x26 for 26.2 Protocol 776) -> Start waiting for level chunks
                protocol::GameEventPacket26_2 game_event{.event_id = 13, .value = 0.0f};
                send_packet(game_event);

                spdlog::info("26.2 PLAY state empty world join sequence completed for player '{}'!", username_);

                // Start periodic 5-second KeepAlive ticker
                start_keep_alive_timer();
            }
        }
        // STATE 3: PLAY
        else if (state_ == protocol::ProtocolState::Play) {
            if (packet_id == 0x00) { // Confirm Teleportation (Serverbound 0x00 in 26.2 PLAY)
                auto confirm_tp_id = buf.read_varint();
                if (confirm_tp_id) {
                    teleport_confirmed_ = true;
                    teleport_confirm_timer_.cancel();
                    player_lifecycle_ = PlayerLifecycleState::SPAWNED_AND_READY;
                    spdlog::info("[Network IN] Confirm Teleportation (0x00) received from player '{}' for Teleport ID {}. Player State: SPAWNED_AND_READY!",
                                 username_, *confirm_tp_id);
                }
            } else if (packet_id == 0x1C) { // Keep Alive Response (Serverbound 0x1C in 26.2 PLAY)
                auto keep_alive_id = buf.read_i64();
                if (keep_alive_id) {
                    last_keep_alive_timestamp_ = std::chrono::steady_clock::now();
                    spdlog::info("[Network IN] Keep Alive response (0x1C) received from player '{}' (ID {})",
                                 username_, *keep_alive_id);
                }
            } else if (packet_id == 0x0E) { // Client Information (Serverbound 0x0E in 26.2 PLAY)
                spdlog::info("[Network IN] Client Information (0x0E) received from player '{}'", username_);
            }
        }
    }

    asio::ip::tcp::socket socket_;
    bool offline_mode_{true};
    protocol::ProtocolState state_{protocol::ProtocolState::Handshake};
    PlayerLifecycleState player_lifecycle_{PlayerLifecycleState::JOINING};
    std::string username_;
    protocol::EncryptionCipher cipher_;
    asio::steady_timer keep_alive_timer_;
    asio::steady_timer teleport_confirm_timer_;
    bool teleport_confirmed_{false};
    std::chrono::steady_clock::time_point last_keep_alive_timestamp_;
    std::array<uint8_t, 512> read_buffer_{};
    std::vector<uint8_t> packet_payload_buffer_;
};

} // namespace papermc::core::network

#endif // PAPERMC_CORE_NETWORK_CONNECTION_HPP
