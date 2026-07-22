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
    static void serialize_section(const ChunkSection& section, protocol::ByteBuf& buf) {
        // 1. Non-air block count: int16_t (Int16BE)
        buf.write_u16(section.non_air_count());

        // 2. Block States Paletted Container
        if (section.non_air_count() == 0) {
            buf.write_u8(0);      // 0 bits per entry
            buf.write_varint(0);  // Palette value: 0 (minecraft:air)
            buf.write_varint(0);  // Data array length: 0
        } else {
            // Dynamic palette extraction for unique block states in section
            std::vector<int32_t> palette;
            palette.push_back(0); // Index 0: Air
            
            std::array<uint8_t, 4096> block_palette_indices{};
            for (int y = 0; y < 16; ++y) {
                for (int z = 0; z < 16; ++z) {
                    for (int x = 0; x < 16; ++x) {
                        auto block_res = section.get_block(x, y, z);
                        int32_t state_id = block_res ? block_res->state_id : 0;
                        uint8_t idx = 0;
                        if (state_id != 0) {
                            auto it = std::find(palette.begin(), palette.end(), state_id);
                            if (it != palette.end()) {
                                idx = static_cast<uint8_t>(std::distance(palette.begin(), it));
                            } else {
                                idx = static_cast<uint8_t>(palette.size());
                                palette.push_back(state_id);
                            }
                        }
                        int block_idx = (y * 256) + (z * 16) + x;
                        block_palette_indices[block_idx] = idx;
                    }
                }
            }

            if (palette.size() == 1) {
                // All same block
                buf.write_u8(0);
                buf.write_varint(palette[0]);
                buf.write_varint(0);
            } else {
                // Dynamic Bits Per Entry (bpe = 4 to 8)
                uint8_t bpe = 4;
                if (palette.size() > 16) bpe = 5;
                if (palette.size() > 32) bpe = 6;
                if (palette.size() > 64) bpe = 7;
                if (palette.size() > 128) bpe = 8;

                buf.write_u8(bpe);
                buf.write_varint(static_cast<int32_t>(palette.size()));
                for (int32_t state_id : palette) {
                    buf.write_varint(state_id);
                }

                int entries_per_long = 64 / bpe;
                int num_longs = (4096 + entries_per_long - 1) / entries_per_long;
                buf.write_varint(num_longs);

                uint64_t current_long = 0;
                int bits_in_long = 0;
                for (int i = 0; i < 4096; ++i) {
                    uint64_t val = block_palette_indices[i] & ((1ULL << bpe) - 1);
                    current_long |= (val << bits_in_long);
                    bits_in_long += bpe;
                    if (bits_in_long + bpe > 64 || i == 4095) {
                        buf.write_i64(static_cast<int64_t>(current_long));
                        current_long = 0;
                        bits_in_long = 0;
                    }
                }
            }
        }

        // 3. Biomes Paletted Container (Single Value Palette, 0 bits/entry)
        buf.write_u8(0);      // 0 bits per entry
        buf.write_varint(0);  // Palette value: 0 (minecraft:plains)
        buf.write_varint(0);  // Data array length: 0
    }

    static void serialize_chunk_data(const ChunkColumn& chunk, protocol::ByteBuf& buf) {
        buf.write_varint(0x2D); // level_chunk_with_light Packet ID (0x2D / 45 in 26.2 Protocol 776)
        buf.write_i32(chunk.x());
        buf.write_i32(chunk.z());

        // 1. Heightmaps NBT Compound Tag (Empty CompoundTag 0x0A 0x00)
        buf.write_u8(0x0A); // TAG_Compound
        buf.write_u8(0x00); // TAG_End

        // 2. Serialized Chunk Sections Payload Buffer (24 sections)
        protocol::ByteBuf section_buf(4096);
        for (std::size_t s = 0; s < CHUNK_SECTIONS; ++s) {
            serialize_section(chunk.get_section(s), section_buf);
        }

        auto data_span = section_buf.data_span();
        buf.write_varint(static_cast<int32_t>(data_span.size()));
        buf.write_bytes(data_span);

        // 3. Block Entities Array Count: VarInt = 0
        buf.write_varint(0);

        // 4. Light Data BitSets & Arrays
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
