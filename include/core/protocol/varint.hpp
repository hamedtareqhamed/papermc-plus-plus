#ifndef PAPERMC_CORE_PROTOCOL_VARINT_HPP
#ifndef PAPERMC_CORE_PROTOCOL_VARINT_HPP
#define PAPERMC_CORE_PROTOCOL_VARINT_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <utility>

namespace papermc::core::protocol {

struct VarIntResult {
    int32_t value;
    std::size_t bytes_read;
};

struct VarLongResult {
    int64_t value;
    std::size_t bytes_read;
};

enum class VarIntError {
    BufferTooSmall,
    VarIntTooBig
};

/**
 * Decodes a 32-bit VarInt from a non-owning byte span using C++23 monadic std::expected.
 */
constexpr std::expected<VarIntResult, std::string_view> decode_varint(std::span<const std::byte> buffer) noexcept {
    int32_t value = 0;
    std::size_t position = 0;
    uint8_t current_byte = 0;

    while (true) {
        if (position >= buffer.size()) {
            return std::unexpected("VarInt decoding error: Buffer read out of bounds");
        }

        current_byte = static_cast<uint8_t>(buffer[position]);
        value |= static_cast<int32_t>(current_byte & 0x7F) << (position * 7);

        position++;
        if (position > 5) {
            return std::unexpected("VarInt decoding error: VarInt is too big (exceeds 5 bytes)");
        }

        if ((current_byte & 0x80) == 0) {
            break;
        }
    }

    return VarIntResult{value, position};
}

/**
 * Encodes a 32-bit integer into a VarInt byte span.
 * Returns the number of bytes written or an error string_view if buffer space is insufficient.
 */
constexpr std::expected<std::size_t, std::string_view> encode_varint(int32_t value, std::span<std::byte> destination) noexcept {
    uint32_t uval = static_cast<uint32_t>(value);
    std::size_t count = 0;

    do {
        if (count >= destination.size()) {
            return std::unexpected("VarInt encoding error: Destination buffer too small");
        }

        uint8_t temp = static_cast<uint8_t>(uval & 0x7F);
        uval >>= 7;

        if (uval != 0) {
            temp |= 0x80;
        }

        destination[count++] = static_cast<std::byte>(temp);
    } while (uval != 0);

    return count;
}

/**
 * Decodes a 64-bit VarLong from a byte span.
 */
constexpr std::expected<VarLongResult, std::string_view> decode_varlong(std::span<const std::byte> buffer) noexcept {
    int64_t value = 0;
    std::size_t position = 0;
    uint8_t current_byte = 0;

    while (true) {
        if (position >= buffer.size()) {
            return std::unexpected("VarLong decoding error: Buffer read out of bounds");
        }

        current_byte = static_cast<uint8_t>(buffer[position]);
        value |= static_cast<int64_t>(current_byte & 0x7F) << (position * 7);

        position++;
        if (position > 10) {
            return std::unexpected("VarLong decoding error: VarLong is too big (exceeds 10 bytes)");
        }

        if ((current_byte & 0x80) == 0) {
            break;
        }
    }

    return VarLongResult{value, position};
}

} // namespace papermc::core::protocol

#endif // PAPERMC_CORE_PROTOCOL_VARINT_HPP
