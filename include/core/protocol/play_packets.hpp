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

// Change Difficulty Packet (Clientbound ID 0x0B for Protocol 765 / 1.20.4)
struct ChangeDifficultyPacket {
    uint8_t difficulty{2}; // 2 = Normal
    bool locked{false};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x0B); // Packet ID 0x0B
        buf.write_u8(difficulty);
        buf.write_u8(locked ? 1 : 0);
    }
};

// Chunk Batch Start Packet (Clientbound ID 0x0D for Protocol 765 / 1.20.4)
struct ChunkBatchStartPacket {
    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x0D); // Packet ID 0x0D (chunk_batch_start)
    }
};

// Chunk Batch Finished Packet (Clientbound ID 0x0C for Protocol 765 / 1.20.4)
struct ChunkBatchFinishedPacket {
    int32_t batch_size{1};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x0C); // Packet ID 0x0C (chunk_batch_finished)
        buf.write_varint(batch_size);
    }
};

// Client Information Packet (Serverbound ID 0x09 in PLAY state for Protocol 765)
struct ClientInformationPacket {
    std::string locale{"en_US"};
    int8_t view_distance{10};
    int32_t chat_mode{0};
    bool chat_colors{true};
    uint8_t displayed_skin_parts{0x7F};
    int32_t main_hand{1};
    bool enable_text_filtering{false};
    bool allow_server_listings{true};

    static std::expected<ClientInformationPacket, std::string_view> deserialize(ByteBuf& buf) noexcept {
        ClientInformationPacket pkt;
        auto loc = buf.read_string();
        if (!loc) return std::unexpected(loc.error());
        pkt.locale = *loc;

        auto vd = buf.read_u8();
        if (!vd) return std::unexpected(vd.error());
        pkt.view_distance = static_cast<int8_t>(*vd);

        auto cm = buf.read_varint();
        if (!cm) return std::unexpected(cm.error());
        pkt.chat_mode = *cm;

        auto cc = buf.read_u8();
        if (!cc) return std::unexpected(cc.error());
        pkt.chat_colors = (*cc != 0);

        auto sp = buf.read_u8();
        if (!sp) return std::unexpected(sp.error());
        pkt.displayed_skin_parts = *sp;

        auto mh = buf.read_varint();
        if (!mh) return std::unexpected(mh.error());
        pkt.main_hand = *mh;

        auto tf = buf.read_u8();
        if (!tf) return std::unexpected(tf.error());
        pkt.enable_text_filtering = (*tf != 0);

        auto sl = buf.read_u8();
        if (!sl) return std::unexpected(sl.error());
        pkt.allow_server_listings = (*sl != 0);

        return pkt;
    }
};

// Game Event Packet (Clientbound ID 0x20 for Protocol 765 / 1.20.4)
struct GameEventPacket {
    uint8_t event_id{13}; // 13 = Start Waiting for Level Chunks
    float value{0.0f};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x20); // Packet ID 0x20
        buf.write_u8(event_id);
        buf.write_i32(std::bit_cast<int32_t>(value));
    }
};

// Update Health Packet (Clientbound ID 0x5B for Protocol 765 / 1.20.4)
struct UpdateHealthPacket {
    float health{20.0f};
    int32_t food{20};
    float food_saturation{5.0f};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x5B); // Packet ID 0x5B
        buf.write_i32(std::bit_cast<int32_t>(health));
        buf.write_varint(food);
        buf.write_i32(std::bit_cast<int32_t>(food_saturation));
    }
};

// Set Center Chunk Packet (Clientbound ID 0x52 for Protocol 765 / 1.20.4)
struct SetCenterChunkPacket {
    int32_t chunk_x{0};
    int32_t chunk_z{0};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x52); // Packet ID 0x52 (update_view_position)
        buf.write_varint(chunk_x);
        buf.write_varint(chunk_z);
    }
};

// Set Default Spawn Position Packet (Clientbound ID 0x54 for Protocol 765 / 1.20.4)
struct SetDefaultSpawnPositionPacket {
    int32_t x{0};
    int32_t y{64};
    int32_t z{0};
    float angle{0.0f};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x54); // Packet ID 0x54 (spawn_position)
        uint64_t val = (static_cast<uint64_t>(x & 0x3FFFFFF) << 38) |
                       (static_cast<uint64_t>(z & 0x3FFFFFF) << 12) |
                       (static_cast<uint64_t>(y & 0xFFF));
        buf.write_i64(static_cast<int64_t>(val));
        buf.write_i32(std::bit_cast<int32_t>(angle));
    }
};

// Join Game Packet (Clientbound ID 0x29 for Protocol 765 / 1.20.4)
struct JoinGamePacket {
    int32_t entity_id{1};
    bool is_hardcore{false};
    std::vector<std::string> dimension_names{"minecraft:overworld"};
    int32_t max_players{100};
    int32_t view_distance{10};
    int32_t simulation_distance{10};
    bool reduced_debug_info{false};
    bool enable_respawn_screen{true};
    bool do_limited_crafting{false};
    std::string dimension_type{"minecraft:overworld"}; // worldType
    std::string dimension_name{"minecraft:overworld"}; // worldName
    int64_t hashed_seed{0};
    uint8_t game_mode{1}; // 1 = Creative, 0 = Survival
    int8_t previous_game_mode{-1};
    bool is_debug{false};
    bool is_flat{true};
    bool has_death_location{false};
    int32_t portal_cooldown{0};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x29); // Packet ID
        buf.write_i32(entity_id);
        buf.write_u8(is_hardcore ? 1 : 0);

        // 1.20.4 worldNames array
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
        buf.write_string(dimension_type);
        buf.write_string(dimension_name);
        buf.write_i64(hashed_seed);
        buf.write_u8(game_mode);
        buf.write_u8(static_cast<uint8_t>(previous_game_mode));
        buf.write_u8(is_debug ? 1 : 0);
        buf.write_u8(is_flat ? 1 : 0);
        
        // Death location option (0 = false)
        buf.write_u8(has_death_location ? 1 : 0);
        if (has_death_location) {
            buf.write_string("minecraft:overworld");
            buf.write_i64(0);
        }

        // Portal cooldown
        buf.write_varint(portal_cooldown);
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
