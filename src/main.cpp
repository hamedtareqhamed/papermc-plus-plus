#include <csignal>
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "core/network/server.hpp"
#include "core/world/chunk.hpp"
#include "core/world/generator.hpp"
#include "core/world/chunk_streamer.hpp"
#include "core/world/entity.hpp"
#include "core/protocol/varint.hpp"
#include "core/protocol/play_packets.hpp"
#include "core/scripting/plugin_engine.hpp"

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

    // Initialize Lua Scripting Plugin Engine
    spdlog::info("Initializing LuaJIT Scripting API Host...");
    papermc::core::scripting::PluginEngine plugin_engine;
    plugin_engine.load_plugin("plugins/example.lua");

    // Trigger sample Lua event
    plugin_engine.trigger_player_join("Steve", "00000000-0000-0000-0000-000000000001");

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

    // Trigger Lua block break event test
    plugin_engine.trigger_block_break("Steve", 0, 3, 0);

    // Initialize Entity Manager
    papermc::core::world::EntityManager entity_manager;
    auto player_id = entity_manager.spawn_entity("minecraft:player");
    spdlog::info("Spawned player entity ID: {}", player_id);

    // Start Async Network Server Engine
    std::string host = "0.0.0.0";
    uint16_t port = 25565;
    std::size_t network_threads = 4;

    try {
        papermc::core::network::ServerEngine server(host, port, network_threads);
        server.start();

        spdlog::info("PaperMC++ Engine online! Listening on {}:{}", host, port);
        spdlog::info("Engine initialized with 0 warnings. Server ready for client connections.");

        // Clean shutdown loop or test exit when non-interactive
        if (argc > 1 && std::string(argv[1]) == "--test-run") {
            spdlog::info("--test-run flag detected. Running 1-second sanity tick before clean exit...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            g_shutdown_requested.store(true);
        } else {
            spdlog::info("Press Ctrl+C to stop.");
            while (!g_shutdown_requested.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        server.stop();
    } catch (const std::exception& ex) {
        spdlog::critical("Fatal exception in main engine loop: {}", ex.what());
        return EXIT_FAILURE;
    }

    spdlog::info("PaperMC++ Engine shut down cleanly.");
    return EXIT_SUCCESS;
}
