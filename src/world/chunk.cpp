#include "core/world/chunk.hpp"
#include <mutex>

namespace papermc::core::world {

std::expected<BlockState, std::string_view> ChunkSection::get_block(std::size_t x, std::size_t y, std::size_t z) const noexcept {
    if (x >= SECTION_WIDTH || y >= SECTION_HEIGHT || z >= SECTION_WIDTH) {
        return std::unexpected("ChunkSection index out of bounds");
    }
    std::size_t index = (y * SECTION_WIDTH * SECTION_WIDTH) + (z * SECTION_WIDTH) + x;
    return blocks_[index];
}

std::expected<void, std::string_view> ChunkSection::set_block(std::size_t x, std::size_t y, std::size_t z, BlockState state) noexcept {
    if (x >= SECTION_WIDTH || y >= SECTION_HEIGHT || z >= SECTION_WIDTH) {
        return std::unexpected("ChunkSection index out of bounds");
    }
    std::size_t index = (y * SECTION_WIDTH * SECTION_WIDTH) + (z * SECTION_WIDTH) + x;
    bool was_air = blocks_[index].is_air();
    bool is_now_air = state.is_air();

    if (was_air && !is_now_air) {
        non_air_count_++;
    } else if (!was_air && is_now_air && non_air_count_ > 0) {
        non_air_count_--;
    }

    blocks_[index] = state;
    return {};
}

ChunkColumn::ChunkColumn(int32_t chunk_x, int32_t chunk_z)
    : chunk_x_(chunk_x), chunk_z_(chunk_z) {}

std::expected<BlockState, std::string_view> ChunkColumn::get_block(int32_t x, int32_t y, int32_t z) const noexcept {
    std::shared_lock lock(chunk_mutex_);
    if (y < -64 || y >= 320) {
        return std::unexpected("Y coordinate out of Minecraft build bounds (-64 to 319)");
    }
    std::size_t section_idx = static_cast<std::size_t>((y + 64) / 16);
    std::size_t local_x = static_cast<std::size_t>(x & 15);
    std::size_t local_y = static_cast<std::size_t>((y + 64) % 16);
    std::size_t local_z = static_cast<std::size_t>(z & 15);

    return sections_[section_idx].get_block(local_x, local_y, local_z);
}

std::expected<void, std::string_view> ChunkColumn::set_block(int32_t x, int32_t y, int32_t z, BlockState state) noexcept {
    std::unique_lock lock(chunk_mutex_);
    if (y < -64 || y >= 320) {
        return std::unexpected("Y coordinate out of Minecraft build bounds (-64 to 319)");
    }
    std::size_t section_idx = static_cast<std::size_t>((y + 64) / 16);
    std::size_t local_x = static_cast<std::size_t>(x & 15);
    std::size_t local_y = static_cast<std::size_t>((y + 64) % 16);
    std::size_t local_z = static_cast<std::size_t>(z & 15);

    return sections_[section_idx].set_block(local_x, local_y, local_z, state);
}

} // namespace papermc::core::world
