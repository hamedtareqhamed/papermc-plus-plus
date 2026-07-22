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
        buf.write_varint(0x00); // Packet ID for Handshake
        buf.write_varint(protocol_version);
        buf.write_string(server_address);
        buf.write_u16(server_port);
        buf.write_varint(next_state);
    }
};

struct StatusRequestPacket {
    static std::expected<StatusRequestPacket, std::string_view> deserialize(ByteBuf& /*buf*/) noexcept {
        return StatusRequestPacket{};
    }
};

struct StatusResponsePacket {
    std::string json_payload;

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x00); // Packet ID for Status Response
        buf.write_string(json_payload);
    }
};

struct PingPacket {
    int64_t payload{0};

    static std::expected<PingPacket, std::string_view> deserialize(ByteBuf& buf) noexcept {
        auto p = buf.read_i64();
        if (!p) return std::unexpected(p.error());
        return PingPacket{.payload = *p};
    }

    void serialize(ByteBuf& buf) const {
        buf.write_varint(0x01); // Packet ID for Pong
        buf.write_i64(payload);
    }
};

} // namespace papermc::core::protocol

#endif // PAPERMC_CORE_PROTOCOL_PACKET_HPP
