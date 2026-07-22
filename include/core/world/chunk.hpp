#ifndef PAPERMC_CORE_WORLD_CHUNK_HPP
#define PAPERMC_CORE_WORLD_CHUNK_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <shared_mutex>
#include <expected>
#include <string_view>
#include "core/world/block.hpp"

namespace papermc::core::world {

constexpr std::size_t SECTION_WIDTH = 16;
constexpr std::size_t SECTION_HEIGHT = 16;
constexpr std::size_t SECTION_BLOCKS = SECTION_WIDTH * SECTION_WIDTH * SECTION_HEIGHT; // 4096 blocks
constexpr std::size_t CHUNK_SECTIONS = 24; // Covers y = -64 to y = 320

/**
 * 16x16x16 Sub-chunk section stored with 64-byte alignment to fit modern L1/L2 cache lines.
 */
class alignas(64) ChunkSection {
public:
    ChunkSection() = default;

    [[nodiscard]] std::expected<BlockState, std::string_view> get_block(std::size_t x, std::size_t y, std::size_t z) const noexcept;
    std::expected<void, std::string_view> set_block(std::size_t x, std::size_t y, std::size_t z, BlockState state) noexcept;

    [[nodiscard]] uint16_t non_air_count() const noexcept { return non_air_count_; }
    [[nodiscard]] std::span<const BlockState, SECTION_BLOCKS> raw_blocks() const noexcept { return blocks_; }

private:
    std::array<BlockState, SECTION_BLOCKS> blocks_{};
    uint16_t non_air_count_{0};
};

/**
 * Full Chunk Column (16x384x16) composed of 24 ChunkSections.
 */
class ChunkColumn {
public:
    ChunkColumn(int32_t chunk_x, int32_t chunk_z);

    [[nodiscard]] int32_t x() const noexcept { return chunk_x_; }
    [[nodiscard]] int32_t z() const noexcept { return chunk_z_; }

    [[nodiscard]] std::expected<BlockState, std::string_view> get_block(int32_t x, int32_t y, int32_t z) const noexcept;
    std::expected<void, std::string_view> set_block(int32_t x, int32_t y, int32_t z, BlockState state) noexcept;

    [[nodiscard]] const ChunkSection& get_section(std::size_t index) const { return sections_[index]; }

private:
    int32_t chunk_x_;
    int32_t chunk_z_;
    mutable std::shared_mutex chunk_mutex_;
    std::array<ChunkSection, CHUNK_SECTIONS> sections_{};
};

} // namespace papermc::core::world

#endif // PAPERMC_CORE_WORLD_CHUNK_HPP
