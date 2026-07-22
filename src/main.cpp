#include <csignal>
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "core/network/server.hpp"
#include "core/world/chunk.hpp"
#include "core/world/entity.hpp"
#include "core/protocol/varint.hpp"

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
    spdlog::info("  PaperMC++ Minecraft Java Engine Core (C++23 Engine)");
    spdlog::info("=========================================================");

    // Register OS termination signals
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize World Chunk Column Demo
    spdlog::info("Initializing Data-Oriented Chunk Column memory pool...");
    papermc::core::world::ChunkColumn chunk_spawn(0, 0);
    
    // Set bedrock at bottom y = -64
    papermc::core::world::BlockState bedrock{.state_id = 1};
    auto set_res = chunk_spawn.set_block(0, -64, 0, bedrock);
    if (set_res) {
        spdlog::info("Set bedrock block at (0, -64, 0) successfully.");
    }

    // Verify block lookup
    auto block_res = chunk_spawn.get_block(0, -64, 0);
    if (block_res) {
        spdlog::info("Retrieved block state ID at (0, -64, 0): {}", block_res->state_id);
    }

    // Initialize Entity Manager Demo
    papermc::core::world::EntityManager entity_manager;
    auto player_id = entity_manager.spawn_entity("minecraft:player");
    spdlog::info("Spawned entity ID: {} (type: minecraft:player)", player_id);

    // Start Async Network Server Engine
    std::string host = "0.0.0.0";
    uint16_t port = 25565;
    std::size_t network_threads = 4;

    try {
        papermc::core::network::ServerEngine server(host, port, network_threads);
        server.start();

        spdlog::info("PaperMC++ Engine online! Listening on {}:{}", host, port);
        spdlog::info("Press Ctrl+C to stop.");

        while (!g_shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        server.stop();
    } catch (const std::exception& ex) {
        spdlog::critical("Fatal exception in main engine loop: {}", ex.what());
        return EXIT_FAILURE;
    }

    spdlog::info("PaperMC++ Engine shutdown complete cleanly.");
    return EXIT_SUCCESS;
}
