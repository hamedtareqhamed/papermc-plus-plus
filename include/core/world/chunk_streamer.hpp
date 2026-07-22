#ifndef PAPERMC_CORE_WORLD_CHUNK_STREAMER_HPP
#define PAPERMC_CORE_WORLD_CHUNK_STREAMER_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <string_view>
#include "core/protocol/buffer.hpp"
#include "core/world/chunk.hpp"
#include "core/world/generator.hpp"

namespace papermc::core::world {

/**
 * Encapsulates Chunk Data & Light packet serialization (Clientbound ID 0x25 for Protocol 765 / 1.20.4).
 */
class ChunkStreamer {
public:
    static void serialize_chunk_data(const ChunkColumn& chunk, protocol::ByteBuf& buf) {
        buf.write_varint(0x25); // Chunk Data and Light Packet ID (0x25 in Protocol 765 / 1.20.4)
        buf.write_i32(chunk.x());
        buf.write_i32(chunk.z());

        // 1. Heightmaps NBT Compound Tag containing MOTION_BLOCKING (37 longs)
        buf.write_u8(0x0A); // TAG_Compound
        buf.write_u16(0);    // Root name length = 0
        buf.write_u8(0x0C); // TAG_Long_Array
        std::string_view tag_name = "MOTION_BLOCKING";
        buf.write_u16(static_cast<uint16_t>(tag_name.size()));
        buf.write_bytes(std::as_bytes(std::span(tag_name)));
        buf.write_i32(37);   // 37 longs for 256 height values (9 bits per entry)
        for (int i = 0; i < 37; ++i) {
            buf.write_i64(0);
        }
        buf.write_u8(0x00); // TAG_End

        // 2. Serialized Chunk Sections Payload Buffer (24 sections)
        protocol::ByteBuf section_buf(4096);
        for (std::size_t s = 0; s < CHUNK_SECTIONS; ++s) {
            const auto& section = chunk.get_section(s);
            
            // a) Non-air block count: int16_t (Int16BE)
            section_buf.write_u16(section.non_air_count());

            // b) Block States Paletted Container (Single Value Palette, 0 bits/entry)
            section_buf.write_u8(0);      // 0 bits per entry
            section_buf.write_varint(0);  // Palette value: 0 (minecraft:air)
            section_buf.write_u8(0);      // Data array length: 0

            // c) Biomes Paletted Container (Single Value Palette, 0 bits/entry)
            section_buf.write_u8(0);      // 0 bits per entry
            section_buf.write_varint(0);  // Palette value: 0 (minecraft:plains)
            section_buf.write_u8(0);      // Data array length: 0
        }

        auto data_span = section_buf.data_span();
        buf.write_varint(static_cast<int32_t>(data_span.size()));
        buf.write_bytes(data_span);

        // 3. Block Entities Array Count: VarInt = 0
        buf.write_varint(0);

        // 4. Light Data BitSets & Arrays (Protocol 765 format)
        buf.write_varint(0); // skyLightMask array count = 0
        buf.write_varint(0); // blockLightMask array count = 0
        buf.write_varint(0); // emptySkyLightMask array count = 0
        buf.write_varint(0); // emptyBlockLightMask array count = 0
        buf.write_varint(0); // skyLight arrays count = 0
        buf.write_varint(0); // blockLight arrays count = 0
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
