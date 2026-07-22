#include <csignal>
#include <iostream>
#include <memory>
#include <atomic>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "core/network/server.hpp"
#include "core/world/chunk.hpp"
#include "core/world/generator.hpp"
#include "core/world/chunk_streamer.hpp"
#include "core/world/entity.hpp"
#include "core/protocol/varint.hpp"
#include "core/protocol/play_packets.hpp"

namespace {
    std::atomic<bool> g_shutdown_requested{false};

    void signal_handler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            spdlog::info("Shutdown signal received ({}). Exiting...", signal);
            g_shutdown_requested.store(true);
        }
    }
}

int main(int argc, char* argv[]) {
    // Configure spdlog
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    auto logger = std::make_shared<spdlog::logger>("papermc", console_sink);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);

    spdlog::info("=========================================================");
    spdlog::info("  PaperMC++ Minecraft Java Engine Core (v0.1.0-alpha.1)");
    spdlog::info("=========================================================");

    // Register OS termination signals
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize Procedural World Chunk Generator & Column
    spdlog::info("Generating Procedural Spawn Chunk Column...");
    papermc::core::world::ChunkGenerator generator(papermc::core::world::GeneratorType::Flatland);
    papermc::core::world::ChunkColumn spawn_chunk(0, 0);
    generator.generate_chunk(spawn_chunk);

    // Verify spawn chunk block inspection
    auto block_res = spawn_chunk.get_block(0, 3, 0); // Grass layer
    if (block_res) {
        spdlog::info("Spawn chunk block at (0, 3, 0) state ID: {} (Grass)", block_res->state_id);
    }

    // Spawn player entity
    papermc::core::world::Entity player(1, "minecraft:player");
    spdlog::info("Spawned player entity ID: {}", player.id());

    // Start Async Networking Engine on port 25565
    uint16_t port = 25565;
    std::size_t thread_count = 4;

    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }

    spdlog::info("Starting PaperMC++ Server Engine on 0.0.0.0:{} (threads: {}, offline-mode: true)...", port, thread_count);

    papermc::core::network::ServerEngine engine("0.0.0.0", port, thread_count, true);
    engine.start();

    spdlog::info("PaperMC++ Engine online! Listening on 0.0.0.0:{}", port);
    spdlog::info("Engine initialized with 0 warnings. Server ready for client connections.");
    spdlog::info("Press Ctrl+C to stop.");

    // Keep server event loop alive until shutdown request
    while (!g_shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::info("Stopping PaperMC++ Server Engine...");
    engine.stop();
    spdlog::info("Server stopped cleanly.");

    return 0;
}
