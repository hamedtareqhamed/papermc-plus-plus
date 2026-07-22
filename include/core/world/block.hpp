#ifndef PAPERMC_CORE_WORLD_BLOCK_HPP
#define PAPERMC_CORE_WORLD_BLOCK_HPP

#include <cstdint>
#include <compare>

namespace papermc::core::world {

/**
 * Packed block state definition optimized for L1 cache alignment.
 */
struct alignas(4) BlockState {
    uint32_t state_id{0}; // 0 = air

    constexpr auto operator<=>(const BlockState&) const = default;

    [[nodiscard]] constexpr bool is_air() const noexcept {
        return state_id == 0;
    }
};

} // namespace papermc::core::world

#endif // PAPERMC_CORE_WORLD_BLOCK_HPP
