#ifndef PAPERMC_CORE_PROTOCOL_BUFFER_HPP
#define PAPERMC_CORE_PROTOCOL_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "core/protocol/varint.hpp"

namespace papermc::core::protocol {

/**
 * Zero-copy capable expandable Byte Buffer for Minecraft Packet protocol serialization.
 */
class ByteBuf {
public:
    ByteBuf() = default;
    explicit ByteBuf(std::size_t initial_capacity);
    explicit ByteBuf(std::span<const std::byte> source);

    [[nodiscard]] std::span<const std::byte> data_span() const noexcept;
    [[nodiscard]] std::span<const std::byte> remaining_span() const noexcept;
    [[nodiscard]] std::size_t readable_bytes() const noexcept;
    [[nodiscard]] std::size_t read_offset() const noexcept;
    [[nodiscard]] std::size_t write_offset() const noexcept;

    void clear() noexcept;

    // Decoding Operations
    std::expected<uint8_t, std::string_view> read_u8() noexcept;
    std::expected<uint16_t, std::string_view> read_u16() noexcept;
    std::expected<int32_t, std::string_view> read_i32() noexcept;
    std::expected<int64_t, std::string_view> read_i64() noexcept;
    std::expected<int32_t, std::string_view> read_varint() noexcept;
    std::expected<std::string, std::string_view> read_string(std::size_t max_len = 32767) noexcept;
    std::expected<std::span<const std::byte>, std::string_view> read_bytes(std::size_t length) noexcept;

    // Encoding Operations
    void write_u8(uint8_t value);
    void write_u16(uint16_t value);
    void write_i32(int32_t value);
    void write_i64(int64_t value);
    void write_varint(int32_t value);
    void write_string(std::string_view str);
    void write_bytes(std::span<const std::byte> bytes);

private:
    std::vector<std::byte> buffer_;
    std::size_t read_cursor_{0};
};

} // namespace papermc::core::protocol

#endif // PAPERMC_CORE_PROTOCOL_BUFFER_HPP
