#include "core/protocol/buffer.hpp"
#include <cstring>
#include <arpa/inet.h>

namespace papermc::core::protocol {

ByteBuf::ByteBuf(std::size_t initial_capacity) {
    buffer_.reserve(initial_capacity);
}

ByteBuf::ByteBuf(std::span<const std::byte> source)
    : buffer_(source.begin(), source.end()) {}

std::span<const std::byte> ByteBuf::data_span() const noexcept {
    return std::span<const std::byte>(buffer_.data(), buffer_.size());
}

std::span<const std::byte> ByteBuf::remaining_span() const noexcept {
    if (read_cursor_ >= buffer_.size()) {
        return {};
    }
    return std::span<const std::byte>(buffer_.data() + read_cursor_, buffer_.size() - read_cursor_);
}

std::size_t ByteBuf::readable_bytes() const noexcept {
    return (read_cursor_ < buffer_.size()) ? (buffer_.size() - read_cursor_) : 0;
}

std::size_t ByteBuf::read_offset() const noexcept {
    return read_cursor_;
}

std::size_t ByteBuf::write_offset() const noexcept {
    return buffer_.size();
}

void ByteBuf::clear() noexcept {
    buffer_.clear();
    read_cursor_ = 0;
}

std::expected<uint8_t, std::string_view> ByteBuf::read_u8() noexcept {
    if (readable_bytes() < 1) {
        return std::unexpected("ByteBuf EOF: Failed to read u8");
    }
    uint8_t val = static_cast<uint8_t>(buffer_[read_cursor_++]);
    return val;
}

std::expected<uint16_t, std::string_view> ByteBuf::read_u16() noexcept {
    if (readable_bytes() < 2) {
        return std::unexpected("ByteBuf EOF: Failed to read u16");
    }
    uint16_t net_val;
    std::memcpy(&net_val, buffer_.data() + read_cursor_, 2);
    read_cursor_ += 2;
    return ntohs(net_val);
}

std::expected<int32_t, std::string_view> ByteBuf::read_i32() noexcept {
    if (readable_bytes() < 4) {
        return std::unexpected("ByteBuf EOF: Failed to read i32");
    }
    uint32_t net_val;
    std::memcpy(&net_val, buffer_.data() + read_cursor_, 4);
    read_cursor_ += 4;
    return static_cast<int32_t>(ntohl(net_val));
}

std::expected<int64_t, std::string_view> ByteBuf::read_i64() noexcept {
    if (readable_bytes() < 8) {
        return std::unexpected("ByteBuf EOF: Failed to read i64");
    }
    uint64_t net_val;
    std::memcpy(&net_val, buffer_.data() + read_cursor_, 8);
    read_cursor_ += 8;

    uint32_t high = ntohl(static_cast<uint32_t>(net_val & 0xFFFFFFFF));
    uint32_t low = ntohl(static_cast<uint32_t>(net_val >> 32));
    uint64_t host_val = (static_cast<uint64_t>(high) << 32) | low;

    return static_cast<int64_t>(host_val);
}

std::expected<int32_t, std::string_view> ByteBuf::read_varint() noexcept {
    auto res = decode_varint(remaining_span());
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }
    read_cursor_ += res->bytes_read;
    return res->value;
}

std::expected<std::string, std::string_view> ByteBuf::read_string(std::size_t max_len) noexcept {
    auto len_res = read_varint();
    if (!len_res.has_value()) {
        return std::unexpected(len_res.error());
    }
    int32_t length = len_res.value();
    if (length < 0 || static_cast<std::size_t>(length) > max_len) {
        return std::unexpected("String length validation failed");
    }
    if (readable_bytes() < static_cast<std::size_t>(length)) {
        return std::unexpected("ByteBuf EOF: Failed to read string bytes");
    }

    std::string str(reinterpret_cast<const char*>(buffer_.data() + read_cursor_), length);
    read_cursor_ += length;
    return str;
}

std::expected<std::span<const std::byte>, std::string_view> ByteBuf::read_bytes(std::size_t length) noexcept {
    if (readable_bytes() < length) {
        return std::unexpected("ByteBuf EOF: Failed to read byte span");
    }
    std::span<const std::byte> slice(buffer_.data() + read_cursor_, length);
    read_cursor_ += length;
    return slice;
}

void ByteBuf::write_u8(uint8_t value) {
    buffer_.push_back(static_cast<std::byte>(value));
}

void ByteBuf::write_u16(uint16_t value) {
    uint16_t net_val = htons(value);
    const auto* p = reinterpret_cast<const std::byte*>(&net_val);
    buffer_.insert(buffer_.end(), p, p + 2);
}

void ByteBuf::write_i32(int32_t value) {
    uint32_t net_val = htonl(static_cast<uint32_t>(value));
    const auto* p = reinterpret_cast<const std::byte*>(&net_val);
    buffer_.insert(buffer_.end(), p, p + 4);
}

void ByteBuf::write_i64(int64_t value) {
    uint64_t val = static_cast<uint64_t>(value);
    uint32_t high = htonl(static_cast<uint32_t>(val >> 32));
    uint32_t low = htonl(static_cast<uint32_t>(val & 0xFFFFFFFF));
    uint64_t net_val = (static_cast<uint64_t>(high)) | (static_cast<uint64_t>(low) << 32);

    const auto* p = reinterpret_cast<const std::byte*>(&net_val);
    buffer_.insert(buffer_.end(), p, p + 8);
}

void ByteBuf::write_varint(int32_t value) {
    std::array<std::byte, 5> temp{};
    auto res = encode_varint(value, std::span<std::byte>(temp.data(), temp.size()));
    if (res.has_value()) {
        buffer_.insert(buffer_.end(), temp.data(), temp.data() + res.value());
    }
}

void ByteBuf::write_string(std::string_view str) {
    write_varint(static_cast<int32_t>(str.size()));
    const auto* p = reinterpret_cast<const std::byte*>(str.data());
    buffer_.insert(buffer_.end(), p, p + str.size());
}

void ByteBuf::write_bytes(std::span<const std::byte> bytes) {
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

} // namespace papermc::core::protocol
