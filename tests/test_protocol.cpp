#include <cassert>
#include <iostream>
#include <vector>
#include <string_view>

#include "core/protocol/varint.hpp"
#include "core/protocol/buffer.hpp"
#include "core/world/chunk.hpp"
#include "core/world/generator.hpp"

void test_varint_codec() {
    std::cout << "[TEST] Running VarInt codec unit test..." << std::endl;
    papermc::core::protocol::ByteBuf buf;
    buf.write_varint(25565);
    buf.write_varint(0);
    buf.write_varint(-1);

    auto val1 = buf.read_varint();
    assert(val1.has_value() && val1.value() == 25565);

    auto val2 = buf.read_varint();
    assert(val2.has_value() && val2.value() == 0);

    auto val3 = buf.read_varint();
    assert(val3.has_value() && val3.value() == -1);

    std::cout << "[SUCCESS] VarInt codec test passed!" << std::endl;
}

void test_chunk_generation() {
    std::cout << "[TEST] Running Chunk generation unit test..." << std::endl;
    papermc::core::world::ChunkColumn chunk(0, 0);
    papermc::core::world::ChunkGenerator generator(papermc::core::world::GeneratorType::Flatland);
    generator.generate_chunk(chunk);

    auto bedrock = chunk.get_block(0, -64, 0);
    assert(bedrock.has_value() && bedrock->state_id == 1);

    auto grass = chunk.get_block(0, 3, 0);
    assert(grass.has_value() && grass->state_id == 3);

    std::cout << "[SUCCESS] Chunk generation unit test passed!" << std::endl;
}

int main() {
    test_varint_codec();
    test_chunk_generation();
    std::cout << "\n=========================================" << std::endl;
    std::cout << " ALL UNIT TESTS PASSED (0 FAILURES)" << std::endl;
    std::cout << "=========================================\n" << std::endl;
    return 0;
}
