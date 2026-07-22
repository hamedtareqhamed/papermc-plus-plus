#ifndef PAPERMC_CORE_PROTOCOL_PACKET_HPP
#define PAPERMC_CORE_PROTOCOL_PACKET_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <utility>
#include <expected>
#include <string_view>
#include <span>
#include <sstream>
#include <iomanip>
#include <bit>
#include "core/protocol/buffer.hpp"

namespace papermc::core::protocol {

inline std::array<uint8_t, 16> parse_uuid_string(std::string_view uuid_str) {
    std::array<uint8_t, 16> bytes{};
    std::string hex;
    for (char c : uuid_str) {
        if (c != '-') hex.push_back(c);
    }
    if (hex.size() == 32) {
        for (std::size_t i = 0; i < 16; ++i) {
            uint32_t val = 0;
            std::string sub = hex.substr(i * 2, 2);
            std::stringstream ss;
            ss << std::hex << sub;
            ss >> val;
            bytes[i] = static_cast<uint8_t>(val);
        }
    } else {
        bytes[15] = 0x01; // Fallback byte
    }
    return bytes;
}

enum class ProtocolState : uint8_t {
    Handshake = 0,
    Status = 1,
    Login = 2,
    Configuration = 4,
    Play = 3
};

// Handshake Packet (State 0, ID 0x00)
struct HandshakePacket {
    int32_t protocol_version{0};
    std::string server_address;
    uint16_t server_port{0};
    int32_t next_state{1}; // 1 = Status, 2 = Login

    static std::expected<HandshakePacket, std::string_view> deserialize(ByteBuf& buf) noexcept {
        HandshakePacket pkt;
        auto version_res = buf.read_varint();
        if (!version_res) return std::unexpected(version_res.error());
        pkt.protocol_version = *version_res;

        auto addr_res = buf.read_string(255);
        if (!addr_res) return std::unexpected(addr_res.error());
        pkt.server_address = std::move(*addr_res);

        auto port_res = buf.read_u16();
        if (!port_res) return std::unexpected(port_res.error());
        pkt.server_port = *port_res;

        auto state_res = buf.read_varint();
        if (!state_res) return std::unexpected(state_res.error());
        pkt.next_state = *state_res;

        return pkt;
    }

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x00);
        buf.write_varint(protocol_version);
        buf.write_string(server_address);
        buf.write_u16(server_port);
        buf.write_varint(next_state);
    }
};

// Status Request Packet (State 1, ID 0x00)
struct StatusRequestPacket {
    static std::expected<StatusRequestPacket, std::string_view> deserialize(ByteBuf& /*buf*/) noexcept {
        return StatusRequestPacket{};
    }
};

// Status Response Packet (State 1, ID 0x00)
struct StatusResponsePacket {
    std::string json_payload;

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x00);
        buf.write_string(json_payload);
    }
};

// Ping / Pong Packet (State 1 & Play, ID 0x01)
struct PingPacket {
    int64_t payload{0};

    static std::expected<PingPacket, std::string_view> deserialize(ByteBuf& buf) noexcept {
        auto p = buf.read_i64();
        if (!p) return std::unexpected(p.error());
        return PingPacket{.payload = *p};
    }

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x01);
        buf.write_i64(payload);
    }
};

// Login Start Packet (State 2, ID 0x00)
struct LoginStartPacket {
    std::string username;
    std::string player_uuid;

    static std::expected<LoginStartPacket, std::string_view> deserialize(ByteBuf& buf) noexcept {
        LoginStartPacket pkt;
        auto name_res = buf.read_string(16);
        if (!name_res) return std::unexpected(name_res.error());
        pkt.username = std::move(*name_res);

        if (buf.readable_bytes() >= 16) {
            auto uuid_bytes = buf.read_bytes(16);
            if (uuid_bytes) {
                pkt.player_uuid = "00000000-0000-0000-0000-000000000001";
            }
        }
        return pkt;
    }
};

// Login Success Packet (State 2, Clientbound ID 0x02 for 26.2 / Protocol 776)
struct LoginSuccessPacket {
    std::array<uint8_t, 16> uuid{};
    std::string username;

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x02); // Packet ID
        buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(uuid.data()), 16));
        buf.write_string(username);
        buf.write_varint(0); // 0 properties
        // Session ID (16 bytes UUID)
        buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(uuid.data()), 16));
    }
};

// Select Known Packs Packet (State CONFIGURATION, Clientbound ID 0x0E for 26.2 / Protocol 776)
struct SelectKnownPacksPacket {
    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x0E); // Clientbound 0x0E in CONFIGURATION state
        buf.write_varint(1);    // 1 pack
        buf.write_string("minecraft");
        buf.write_string("core");
        buf.write_string("26.2");
    }
};

// Unnamed Network NBT Field Writers
inline void write_nbt_string(ByteBuf& buf, std::string_view name, std::string_view value) {
    buf.write_u8(0x08); // TAG_String
    buf.write_u16(static_cast<uint16_t>(name.size()));
    buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(name.data()), name.size()));
    buf.write_u16(static_cast<uint16_t>(value.size()));
    buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(value.data()), value.size()));
}

inline void write_nbt_byte(ByteBuf& buf, std::string_view name, uint8_t value) {
    buf.write_u8(0x01); // TAG_Byte
    buf.write_u16(static_cast<uint16_t>(name.size()));
    buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(name.data()), name.size()));
    buf.write_u8(value);
}

inline void write_nbt_int(ByteBuf& buf, std::string_view name, int32_t value) {
    buf.write_u8(0x03); // TAG_Int
    buf.write_u16(static_cast<uint16_t>(name.size()));
    buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(name.data()), name.size()));
    buf.write_i32(value);
}

inline void write_nbt_float(ByteBuf& buf, std::string_view name, float value) {
    buf.write_u8(0x05); // TAG_Float
    buf.write_u16(static_cast<uint16_t>(name.size()));
    buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(name.data()), name.size()));
    buf.write_i32(std::bit_cast<int32_t>(value));
}

inline void write_nbt_double(ByteBuf& buf, std::string_view name, double value) {
    buf.write_u8(0x06); // TAG_Double
    buf.write_u16(static_cast<uint16_t>(name.size()));
    buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(name.data()), name.size()));
    buf.write_i64(std::bit_cast<int64_t>(value));
}

inline void write_overworld_dimension_nbt(ByteBuf& buf) {
    buf.write_u8(0x0A); // Root TAG_Compound (Unnamed Network NBT)

    write_nbt_float(buf, "ambient_light", 0.0f);
    write_nbt_byte(buf, "bed_works", 1);
    write_nbt_double(buf, "coordinate_scale", 1.0);
    write_nbt_string(buf, "effects", "minecraft:overworld");
    write_nbt_byte(buf, "has_ceiling", 0);
    write_nbt_byte(buf, "has_raids", 1);
    write_nbt_byte(buf, "has_skylight", 1);
    write_nbt_int(buf, "height", 384);
    write_nbt_string(buf, "infiniburn", "#minecraft:infiniburn_overworld");
    write_nbt_int(buf, "logical_height", 384);
    write_nbt_int(buf, "min_y", -64);
    write_nbt_int(buf, "monster_spawn_block_light_limit", 0);

    // monster_spawn_light_level Compound
    buf.write_u8(0x0A); // TAG_Compound
    std::string_view name = "monster_spawn_light_level";
    buf.write_u16(static_cast<uint16_t>(name.size()));
    buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(name.data()), name.size()));
    
    write_nbt_string(buf, "type", "minecraft:uniform");
    
    // value Compound inside monster_spawn_light_level
    buf.write_u8(0x0A); // TAG_Compound
    std::string_view val_name = "value";
    buf.write_u16(static_cast<uint16_t>(val_name.size()));
    buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(val_name.data()), val_name.size()));
    
    write_nbt_int(buf, "min_inclusive", 0);
    write_nbt_int(buf, "max_inclusive", 7);
    buf.write_u8(0x00); // TAG_End for value compound

    buf.write_u8(0x00); // TAG_End for monster_spawn_light_level compound

    write_nbt_byte(buf, "natural", 1);
    write_nbt_byte(buf, "piglin_safe", 0);
    write_nbt_byte(buf, "respawn_anchor_works", 0);
    write_nbt_byte(buf, "ultrawarm", 0);

    buf.write_u8(0x00); // TAG_End for root compound
}

// Registry Data Packet (State CONFIGURATION, Clientbound ID 0x07 for 26.2 / Protocol 776)
struct RegistryDataPacket {
    std::string registry_id;
    std::vector<std::string> entry_ids;
    bool include_overworld_nbt{false};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x07); // Clientbound 0x07 in CONFIGURATION state
        buf.write_string(registry_id);
        buf.write_varint(static_cast<int32_t>(entry_ids.size()));
        for (const auto& entry_id : entry_ids) {
            buf.write_string(entry_id);
            if (include_overworld_nbt && entry_id == "minecraft:overworld") {
                buf.write_u8(1); // has_data = true
                write_overworld_dimension_nbt(buf);
            } else {
                buf.write_u8(0); // has_data = false (sourced from selected known pack)
            }
        }
    }
};

// Update Enabled Features Packet (State CONFIGURATION, Clientbound ID 0x0C for 26.2 / Protocol 776)
struct UpdateEnabledFeaturesPacket {
    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x0C); // Clientbound 0x0C in CONFIGURATION state
        buf.write_varint(1);    // 1 feature flag
        buf.write_string("minecraft:vanilla");
    }
};

// Update Tags Packet (State CONFIGURATION, Clientbound ID 0x0D for 26.2 / Protocol 776)
struct UpdateTagsPacket {
    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x0D); // Clientbound 0x0D in CONFIGURATION state
        
        // 2 Tagged Registries: minecraft:damage_type & minecraft:timeline
        buf.write_varint(2);

        // Registry 1: minecraft:damage_type
        buf.write_string("minecraft:damage_type");
        std::vector<std::pair<std::string, std::vector<int32_t>>> damage_tags = {
            {"minecraft:is_fire", {0, 1, 2}},
            {"minecraft:is_drowning", {4}},
            {"minecraft:is_freezing", {16}},
            {"minecraft:is_fall", {7, 8}},
            {"minecraft:is_explosion", {25}},
            {"minecraft:is_projectile", {19, 20}},
            {"minecraft:bypasses_armor", {5, 7, 9, 11, 12, 13, 14, 16}},
            {"minecraft:bypasses_invulnerability", {9}},
            {"minecraft:bypasses_shield", {3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 26}},
            {"minecraft:burns_armor", {0, 1, 2}},
            {"minecraft:always_hurts_knockback", {21, 22, 23}}
        };

        buf.write_varint(static_cast<int32_t>(damage_tags.size()));
        for (const auto& [tag_name, entries] : damage_tags) {
            buf.write_string(tag_name);
            buf.write_varint(static_cast<int32_t>(entries.size()));
            for (int32_t entry_id : entries) {
                buf.write_varint(entry_id);
            }
        }

        // Registry 2: minecraft:timeline
        buf.write_string("minecraft:timeline");
        std::vector<std::pair<std::string, std::vector<int32_t>>> timeline_tags = {
            {"minecraft:in_overworld", {0}}
        };

        buf.write_varint(static_cast<int32_t>(timeline_tags.size()));
        for (const auto& [tag_name, entries] : timeline_tags) {
            buf.write_string(tag_name);
            buf.write_varint(static_cast<int32_t>(entries.size()));
            for (int32_t entry_id : entries) {
                buf.write_varint(entry_id);
            }
        }
    }
};

// Finish Configuration Packet (State CONFIGURATION, Clientbound ID 0x03 for 26.2 / Protocol 776)
struct FinishConfigurationPacket {
    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x03); // Clientbound 0x03 in CONFIGURATION for 26.2
    }
};

} // namespace papermc::core::protocol

#endif // PAPERMC_CORE_PROTOCOL_PACKET_HPP
