#ifndef PAPERMC_CORE_NETWORK_CONNECTION_HPP
#define PAPERMC_CORE_NETWORK_CONNECTION_HPP

#include <memory>
#include <array>
#include <vector>
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include "core/protocol/packet.hpp"
#include "core/protocol/buffer.hpp"
#include "core/protocol/encryption.hpp"

namespace papermc::core::network {

class ServerConnection : public std::enable_shared_from_this<ServerConnection> {
public:
    explicit ServerConnection(asio::ip::tcp::socket socket)
        : socket_(std::move(socket)) {}

    void start() {
        std::string remote_addr = socket_.remote_endpoint().address().to_string();
        spdlog::info("Client connected from {}", remote_addr);
        do_read_packet_length();
    }

    void close() {
        asio::error_code ec;
        socket_.close(ec);
    }

private:
    void do_read_packet_length() {
        auto self(shared_from_this());
        // Read packet header length (VarInt up to 5 bytes)
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

                // Read remaining packet payload
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
        spdlog::debug("Received Packet ID 0x{:02X} in state {}", packet_id, static_cast<int>(state_));

        if (state_ == protocol::ProtocolState::Handshake && packet_id == 0x00) {
            auto handshake = protocol::HandshakePacket::deserialize(buf);
            if (handshake) {
                spdlog::info("Handshake received! Protocol Version: {}, Address: {}, Port: {}, Next State: {}",
                             handshake->protocol_version, handshake->server_address, handshake->server_port, handshake->next_state);
                state_ = (handshake->next_state == 1) ? protocol::ProtocolState::Status : protocol::ProtocolState::Login;
            }
        }
    }

    asio::ip::tcp::socket socket_;
    protocol::ProtocolState state_{protocol::ProtocolState::Handshake};
    protocol::EncryptionCipher cipher_;
    std::array<uint8_t, 512> read_buffer_{};
    std::vector<uint8_t> packet_payload_buffer_;
};

} // namespace papermc::core::network

#endif // PAPERMC_CORE_NETWORK_CONNECTION_HPP
