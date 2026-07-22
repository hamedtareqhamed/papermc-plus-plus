#ifndef PAPERMC_CORE_SCRIPTING_PLUGIN_ENGINE_HPP
#define PAPERMC_CORE_SCRIPTING_PLUGIN_ENGINE_HPP

#include <string>
#include <vector>
#include <functional>
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>

namespace papermc::core::scripting {

class PluginEngine {
public:
    PluginEngine() {
        lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
        setup_bindings();
    }

    void load_plugin(const std::string& filepath) {
        try {
            sol::load_result script = lua_.load_file(filepath);
            if (!script.valid()) {
                sol::error err = script;
                std::string err_msg = err.what();
                spdlog::error("Lua plugin load error in {}: {}", filepath, err_msg);
                return;
            }
            script();
            spdlog::info("Loaded Lua plugin: {}", filepath);
        } catch (const std::exception& ex) {
            std::string ex_msg = ex.what();
            spdlog::error("Lua plugin execution error in {}: {}", filepath, ex_msg);
        }
    }

    void trigger_player_join(const std::string& username, const std::string& uuid) {
        sol::protected_function func = lua_["onPlayerJoin"];
        if (func.valid()) {
            auto res = func(username, uuid);
            if (!res.valid()) {
                sol::error err = res;
                std::string err_msg = err.what();
                spdlog::error("Error in Lua onPlayerJoin: {}", err_msg);
            }
        }
    }

    void trigger_block_break(const std::string& username, int32_t x, int32_t y, int32_t z) {
        sol::protected_function func = lua_["onBlockBreak"];
        if (func.valid()) {
            auto res = func(username, x, y, z);
            if (!res.valid()) {
                sol::error err = res;
                std::string err_msg = err.what();
                spdlog::error("Error in Lua onBlockBreak: {}", err_msg);
            }
        }
    }

    void trigger_chat_message(const std::string& username, const std::string& message) {
        sol::protected_function func = lua_["onChatMessage"];
        if (func.valid()) {
            auto res = func(username, message);
            if (!res.valid()) {
                sol::error err = res;
                std::string err_msg = err.what();
                spdlog::error("Error in Lua onChatMessage: {}", err_msg);
            }
        }
    }

private:
    void setup_bindings() {
        lua_["log_info"] = [](const std::string& msg) {
            spdlog::info("[LuaPlugin] {}", msg);
        };
        lua_["log_warn"] = [](const std::string& msg) {
            spdlog::warn("[LuaPlugin] {}", msg);
        };
    }

    sol::state lua_;
};

} // namespace papermc::core::scripting

#endif // PAPERMC_CORE_SCRIPTING_PLUGIN_ENGINE_HPP
