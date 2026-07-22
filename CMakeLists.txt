cmake_minimum_required(VERSION 3.25)
project(papermc_core VERSION 1.0.0 LANGUAGES C CXX)

# Force C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization and Warning flags
if(MSVC)
    add_compile_options(/W4 /O2 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
endif()

include(FetchContent)

# 1. Standalone Asio (Networking)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        master
)
set(ASIO_NO_DEPRECATED ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(asio)

# 2. spdlog (Logging)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# 3. nlohmann_json (JSON Parsing)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# 4. Sol2 (Lua C++ Binding framework)
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/TheLartians/Sol2.git
    GIT_TAG        v3.3.0
)
FetchContent_MakeAvailable(sol2)

# Include Directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${asio_SOURCE_DIR}/asio/include
)

# Project Core Sources
set(CORE_SOURCES
    src/main.cpp
    src/protocol/buffer.cpp
    src/world/chunk.cpp
    src/network/server.cpp
)

add_executable(papermc_core ${CORE_SOURCES})

# Target Definitions & Links
target_compile_definitions(papermc_core PRIVATE ASIO_STANDALONE)

target_link_libraries(papermc_core PRIVATE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    sol2
)

# Threading library
find_package(Threads REQUIRED)
target_link_libraries(papermc_core PRIVATE Threads::Threads)
