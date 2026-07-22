#ifndef PAPERMC_CORE_PROTOCOL_PLAY_PACKETS_HPP
#define PAPERMC_CORE_PROTOCOL_PLAY_PACKETS_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <expected>
#include <string_view>
#include "core/protocol/buffer.hpp"

namespace papermc::core::protocol {

// Join Game Packet (Clientbound ID 0x29)
struct JoinGamePacket {
    int32_t entity_id{1};
    bool is_hardcore{false};
    uint8_t game_mode{1}; // 1 = Creative, 0 = Survival
    int8_t previous_game_mode{-1};
    std::vector<std::string> dimension_names{"minecraft:overworld"};
    std::string dimension_type{"minecraft:overworld"};
    std::string dimension_name{"minecraft:overworld"};
    int64_t hashed_seed{0};
    int32_t max_players{100};
    int32_t view_distance{10};
    int32_t simulation_distance{10};
    bool reduced_debug_info{false};
    bool enable_respawn_screen{true};
    bool is_flat{true};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x29); // Join Game Packet ID
        buf.write_i32(entity_id);
        buf.write_u8(is_hardcore ? 1 : 0);
        buf.write_u8(game_mode);
        buf.write_u8(static_cast<uint8_t>(previous_game_mode));

        // Dimension count & array
        buf.write_varint(static_cast<int32_t>(dimension_names.size()));
        for (const auto& name : dimension_names) {
            buf.write_string(name);
        }

        buf.write_string(dimension_type);
        buf.write_string(dimension_name);
        buf.write_i64(hashed_seed);
        buf.write_varint(max_players);
        buf.write_varint(view_distance);
        buf.write_varint(simulation_distance);
        buf.write_u8(reduced_debug_info ? 1 : 0);
        buf.write_u8(enable_respawn_screen ? 1 : 0);
        buf.write_u8(is_flat ? 1 : 0);
    }
};

// Player Position And Look Packet (Clientbound ID 0x3E)
struct PlayerPositionAndLookPacket {
    double x{0.0};
    double y{64.0};
    double z{0.0};
    float yaw{0.0f};
    float pitch{0.0f};
    uint8_t flags{0};
    int32_t teleport_id{1};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x3E);
        buf.write_i64(std::bit_cast<int64_t>(x));
        buf.write_i64(std::bit_cast<int64_t>(y));
        buf.write_i64(std::bit_cast<int64_t>(z));
        buf.write_i32(std::bit_cast<int32_t>(yaw));
        buf.write_i32(std::bit_cast<int32_t>(pitch));
        buf.write_u8(flags);
        buf.write_varint(teleport_id);
    }
};

// Held Item Change Packet (Clientbound 0x49 / Serverbound 0x28)
struct HeldItemChangePacket {
    int16_t slot{0};

    static std::expected<HeldItemChangePacket, std::string_view> deserialize(ByteBuf& buf) noexcept {
        auto s = buf.read_u16();
        if (!s) return std::unexpected(s.error());
        return HeldItemChangePacket{.slot = static_cast<int16_t>(*s)};
    }

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x49);
        buf.write_u8(static_cast<uint8_t>(slot));
    }
};

// Player Abilities Packet (Clientbound ID 0x34)
struct PlayerAbilitiesPacket {
    uint8_t flags{0x06}; // Invulnerable, Flying, Allow Flying, Creative Mode
    float flying_speed{0.05f};
    float fov_modifier{0.1f};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x34);
        buf.write_u8(flags);
        buf.write_i32(std::bit_cast<int32_t>(flying_speed));
        buf.write_i32(std::bit_cast<int32_t>(fov_modifier));
    }
};

// Respawn Packet (Clientbound ID 0x43)
struct RespawnPacket {
    std::string dimension_type{"minecraft:overworld"};
    std::string dimension_name{"minecraft:overworld"};
    int64_t hashed_seed{0};
    uint8_t game_mode{1};
    int8_t previous_game_mode{-1};
    bool is_flat{true};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x43);
        buf.write_string(dimension_type);
        buf.write_string(dimension_name);
        buf.write_i64(hashed_seed);
        buf.write_u8(game_mode);
        buf.write_u8(static_cast<uint8_t>(previous_game_mode));
        buf.write_u8(is_flat ? 1 : 0);
        buf.write_u8(0); // keep data flag
    }
};

} // namespace papermc::core::protocol

#endif // PAPERMC_CORE_PROTOCOL_PLAY_PACKETS_HPP
