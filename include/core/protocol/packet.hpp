#ifndef PAPERMC_CORE_PROTOCOL_PACKET_HPP
#define PAPERMC_CORE_PROTOCOL_PACKET_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <expected>
#include <string_view>
#include "core/protocol/buffer.hpp"

namespace papermc::core::protocol {

enum class ProtocolState : uint8_t {
    Handshake = 0,
    Status = 1,
    Login = 2,
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

        // Has UUID boolean / string if present
        if (buf.readable_bytes() >= 16) {
            auto uuid_bytes = buf.read_bytes(16);
            if (uuid_bytes) {
                pkt.player_uuid = "00000000-0000-0000-0000-000000000000";
            }
        }
        return pkt;
    }
};

// Encryption Request Packet (State 2, ID 0x01)
struct EncryptionRequestPacket {
    std::string server_id;
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> verify_token;

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x01);
        buf.write_string(server_id);
        buf.write_varint(static_cast<int32_t>(public_key.size()));
        buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(public_key.data()), public_key.size()));
        buf.write_varint(static_cast<int32_t>(verify_token.size()));
        buf.write_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(verify_token.data()), verify_token.size()));
    }
};

// Encryption Response Packet (State 2, ID 0x01)
struct EncryptionResponsePacket {
    std::vector<uint8_t> shared_secret;
    std::vector<uint8_t> verify_token;

    static std::expected<EncryptionResponsePacket, std::string_view> deserialize(ByteBuf& buf) noexcept {
        EncryptionResponsePacket pkt;
        auto secret_len = buf.read_varint();
        if (!secret_len || *secret_len <= 0) return std::unexpected("Invalid shared secret length");
        auto secret_bytes = buf.read_bytes(static_cast<std::size_t>(*secret_len));
        if (!secret_bytes) return std::unexpected(secret_bytes.error());
        pkt.shared_secret.assign(reinterpret_cast<const uint8_t*>(secret_bytes->data()), reinterpret_cast<const uint8_t*>(secret_bytes->data() + secret_bytes->size()));

        auto token_len = buf.read_varint();
        if (!token_len || *token_len <= 0) return std::unexpected("Invalid verify token length");
        auto token_bytes = buf.read_bytes(static_cast<std::size_t>(*token_len));
        if (!token_bytes) return std::unexpected(token_bytes.error());
        pkt.verify_token.assign(reinterpret_cast<const uint8_t*>(token_bytes->data()), reinterpret_cast<const uint8_t*>(token_bytes->data() + token_bytes->size()));

        return pkt;
    }
};

// Login Success Packet (State 2, ID 0x02)
struct LoginSuccessPacket {
    std::string uuid;
    std::string username;

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x02);
        buf.write_string(uuid);
        buf.write_string(username);
        buf.write_varint(0); // 0 properties
    }
};

// Set Compression Packet (State 2, ID 0x03)
struct SetCompressionPacket {
    int32_t threshold{256};

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x03);
        buf.write_varint(threshold);
    }
};

// Keep Alive Packet (State 3 Play, ID 0x23 Clientbound / 0x15 Serverbound)
struct KeepAlivePacket {
    int64_t keep_alive_id{0};

    static std::expected<KeepAlivePacket, std::string_view> deserialize(ByteBuf& buf) noexcept {
        auto id_res = buf.read_i64();
        if (!id_res) return std::unexpected(id_res.error());
        return KeepAlivePacket{.keep_alive_id = *id_res};
    }

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x23);
        buf.write_i64(keep_alive_id);
    }
};

} // namespace papermc::core::protocol

#endif // PAPERMC_CORE_PROTOCOL_PACKET_HPP
