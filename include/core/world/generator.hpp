#ifndef PAPERMC_CORE_WORLD_GENERATOR_HPP
#define PAPERMC_CORE_WORLD_GENERATOR_HPP

#include <memory>
#include <cmath>
#include "core/world/chunk.hpp"

namespace papermc::core::world {

enum class GeneratorType {
    Flatland,
    SimplexNoise
};

/**
 * High-performance C++23 Procedural World Generator.
 */
class ChunkGenerator {
public:
    explicit ChunkGenerator(GeneratorType type = GeneratorType::Flatland)
        : type_(type) {}

    void generate_chunk(ChunkColumn& chunk) const {
        BlockState bedrock{.state_id = 85}; // Bedrock
        BlockState dirt{.state_id = 10};    // Dirt
        BlockState grass{.state_id = 9};    // Grass Block
        BlockState stone{.state_id = 1};    // Stone

        for (int32_t x = 0; x < 16; ++x) {
            for (int32_t z = 0; z < 16; ++z) {
                // Bedrock layer
                chunk.set_block(x, -64, z, bedrock);

                if (type_ == GeneratorType::Flatland) {
                    // Stone layers: -63 to -1
                    for (int32_t y = -63; y < 0; ++y) {
                        chunk.set_block(x, y, z, stone);
                    }
                    // Dirt layers: 0 to 3
                    for (int32_t y = 0; y < 3; ++y) {
                        chunk.set_block(x, y, z, dirt);
                    }
                    // Grass top: y = 3
                    chunk.set_block(x, 3, z, grass);
                } else {
                    // Simple noise elevation formula
                    double world_x = (chunk.x() * 16) + x;
                    double world_z = (chunk.z() * 16) + z;
                    int32_t height = static_cast<int32_t>(64.0 + 8.0 * std::sin(world_x * 0.05) * std::cos(world_z * 0.05));

                    for (int32_t y = -63; y < height - 3; ++y) {
                        chunk.set_block(x, y, z, stone);
                    }
                    for (int32_t y = height - 3; y < height; ++y) {
                        chunk.set_block(x, y, z, dirt);
                    }
                    chunk.set_block(x, height, z, grass);
                }
            }
        }
    }

private:
    GeneratorType type_;
};

} // namespace papermc::core::world

#endif // PAPERMC_CORE_WORLD_GENERATOR_HPP
