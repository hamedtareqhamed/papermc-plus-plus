#ifndef PAPERMC_CORE_WORLD_CHUNK_STREAMER_HPP
#define PAPERMC_CORE_WORLD_CHUNK_STREAMER_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include "core/protocol/buffer.hpp"
#include "core/world/chunk.hpp"
#include "core/world/generator.hpp"

namespace papermc::core::world {

/**
 * Encapsulates Chunk Data & Light packet serialization (Clientbound ID 0x22).
 */
class ChunkStreamer {
public:
    static void serialize_chunk_data(const ChunkColumn& chunk, protocol::ByteBuf& buf) {
        buf.write_varint(0x22); // Chunk Data and Light Packet ID
        buf.write_i32(chunk.x());
        buf.write_i32(chunk.z());

        // Heightmap NBT (Simplified Tag Compound)
        buf.write_u8(0x0A); // Compound Tag
        buf.write_u8(0x00); // Empty Name string length 0
        buf.write_u8(0x00);

        // Dummy serialised chunk section binary payload
        protocol::ByteBuf section_buf(2048);
        for (std::size_t s = 0; s < CHUNK_SECTIONS; ++s) {
            const auto& section = chunk.get_section(s);
            section_buf.write_u16(section.non_air_count());
            section_buf.write_u8(0); // Single value palette (0 bits per entry)
            section_buf.write_varint(0); // Air state ID
            section_buf.write_varint(0); // Data array length
        }

        auto data_span = section_buf.data_span();
        buf.write_varint(static_cast<int32_t>(data_span.size()));
        buf.write_bytes(data_span);

        // Block entity count
        buf.write_varint(0);

        // Light Data flags
        buf.write_varint(0); // Sky light mask
        buf.write_varint(0); // Block light mask
        buf.write_varint(0); // Empty sky light mask
        buf.write_varint(0); // Empty block light mask
    }
};

struct BlockChangeEvent {
    int32_t x;
    int32_t y;
    int32_t z;
    BlockState new_state;

    void serialize(protocol::ByteBuf& buf) const {
        buf.write_varint(0x09); // Block Change Packet ID (Clientbound 0x09)
        // Position encoding: (x & 0x3FFFFFF) << 38 | (z & 0x3FFFFFF) << 12 | (y & 0xFFF)
        uint64_t val = (static_cast<uint64_t>(x & 0x3FFFFFF) << 38) |
                       (static_cast<uint64_t>(z & 0x3FFFFFF) << 12) |
                       (static_cast<uint64_t>(y & 0xFFF));
        buf.write_i64(static_cast<int64_t>(val));
        buf.write_varint(static_cast<int32_t>(new_state.state_id));
    }
};

} // namespace papermc::core::world

#endif // PAPERMC_CORE_WORLD_CHUNK_STREAMER_HPP
