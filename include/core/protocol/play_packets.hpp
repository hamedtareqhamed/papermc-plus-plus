#ifndef PAPERMC_CORE_PROTOCOL_PLAY_PACKETS_HPP
#define PAPERMC_CORE_PROTOCOL_PLAY_PACKETS_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <expected>
#include <string_view>
#include <bit>
#include "core/protocol/buffer.hpp"

namespace papermc::core::protocol {

/**
 * 26.2 / Protocol 776 Login (play) Packet (Clientbound ID 0x31 / 49)
 */
struct LoginPlayPacket26_2 {
    int32_t entity_id{1};
    bool is_hardcore{false};
    std::vector<std::string> dimension_names{"minecraft:overworld"};
    int32_t max_players{100};
    int32_t view_distance{10};
    int32_t simulation_distance{10};
    bool reduced_debug_info{false};
    bool enable_respawn_screen{true};
    bool do_limited_crafting{false};
    int32_t dimension_type{0}; // Registry ID 0 (overworld)
    std::string dimension_name{"minecraft:overworld"};
    int64_t hashed_seed{0};
    uint8_t game_mode{1}; // 1 = Creative
    int8_t previous_game_mode{-1};
    bool is_debug{false};
    bool is_flat{true};
    bool has_death_location{false};
    int32_t portal_cooldown{0};
    int32_t sea_level{63};
    bool online_mode{false};
    bool enforces_secure_chat{false};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x31); // Clientbound 0x31 in 26.2 Protocol 776
        buf.write_i32(entity_id);
        buf.write_u8(is_hardcore ? 1 : 0);

        buf.write_varint(static_cast<int32_t>(dimension_names.size()));
        for (const auto& name : dimension_names) {
            buf.write_string(name);
        }

        buf.write_varint(max_players);
        buf.write_varint(view_distance);
        buf.write_varint(simulation_distance);
        buf.write_u8(reduced_debug_info ? 1 : 0);
        buf.write_u8(enable_respawn_screen ? 1 : 0);
        buf.write_u8(do_limited_crafting ? 1 : 0);
        buf.write_varint(dimension_type);
        buf.write_string(dimension_name);
        buf.write_i64(hashed_seed);
        buf.write_u8(game_mode);
        buf.write_u8(static_cast<uint8_t>(previous_game_mode));
        buf.write_u8(is_debug ? 1 : 0);
        buf.write_u8(is_flat ? 1 : 0);
        
        buf.write_u8(has_death_location ? 1 : 0);
        if (has_death_location) {
            buf.write_string("minecraft:overworld");
            buf.write_i64(0);
        }

        buf.write_varint(portal_cooldown);
        buf.write_varint(sea_level);
        buf.write_u8(online_mode ? 1 : 0);
        buf.write_u8(enforces_secure_chat ? 1 : 0);
    }
};

/**
 * 26.2 / Protocol 776 Synchronize Player Position Packet (Clientbound ID 0x48 / 72)
 */
struct PlayerPositionPacket26_2 {
    int32_t teleport_id{1};
    double x{0.0};
    double y{64.0};
    double z{0.0};
    double vx{0.0};
    double vy{0.0};
    double vz{0.0};
    float yaw{0.0f};
    float pitch{0.0f};
    int32_t flags{0};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x48); // Clientbound 0x48 in 26.2 Protocol 776
        buf.write_varint(teleport_id);
        buf.write_i64(std::bit_cast<int64_t>(x));
        buf.write_i64(std::bit_cast<int64_t>(y));
        buf.write_i64(std::bit_cast<int64_t>(z));
        buf.write_i64(std::bit_cast<int64_t>(vx));
        buf.write_i64(std::bit_cast<int64_t>(vy));
        buf.write_i64(std::bit_cast<int64_t>(vz));
        buf.write_i32(std::bit_cast<int32_t>(yaw));
        buf.write_i32(std::bit_cast<int32_t>(pitch));
        buf.write_i32(flags);
    }
};

/**
 * 26.2 / Protocol 776 Game Event Packet (Clientbound ID 0x26 / 38)
 */
struct GameEventPacket26_2 {
    uint8_t event_id{13}; // 13 = Start waiting for level chunks
    float value{0.0f};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x26); // Clientbound 0x26 in 26.2 Protocol 776
        buf.write_u8(event_id);
        buf.write_i32(std::bit_cast<int32_t>(value));
    }
};

/**
 * 26.2 / Protocol 776 Keep Alive Clientbound Packet (Clientbound ID 0x2C / 44)
 */
struct KeepAliveClientbound26_2 {
    int64_t keep_alive_id{0};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x2C); // Clientbound 0x2C in 26.2 Protocol 776
        buf.write_i64(keep_alive_id);
    }
};

// Set Center Chunk Packet (Clientbound 0x59 in PLAY state for 26.2)
struct SetCenterChunkPacket {
    int32_t chunk_x{0};
    int32_t chunk_z{0};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x59);
        buf.write_varint(chunk_x);
        buf.write_varint(chunk_z);
    }
};

} // namespace papermc::core::protocol

#endif // PAPERMC_CORE_PROTOCOL_PLAY_PACKETS_HPP
