cmake_minimum_required(VERSION 3.25)
project(papermc_plus_plus VERSION 0.1.0 LANGUAGES C CXX)

# Force C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization and Warning flags
if(MSVC)
    add_compile_options(/W4 /O2 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-dangling-reference -O3 -march=native)
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

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/core/protocol/generated/registries.hpp
           ${CMAKE_CURRENT_SOURCE_DIR}/include/core/protocol/generated/tags.hpp
           ${CMAKE_CURRENT_SOURCE_DIR}/include/core/protocol/generated/packets.hpp
           ${CMAKE_CURRENT_SOURCE_DIR}/include/core/protocol/generated/blocks.hpp
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_all_mc_headers.py
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_all_mc_headers.py
    COMMENT "Generating 100% MC headers from JSON..."
)
add_custom_target(generate_registries DEPENDS 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/protocol/generated/registries.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/protocol/generated/tags.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/protocol/generated/packets.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/protocol/generated/blocks.hpp
)

add_executable(papermc_plus_plus ${CORE_SOURCES})
add_dependencies(papermc_plus_plus generate_registries)

# Target Definitions & Links
target_compile_definitions(papermc_plus_plus PRIVATE ASIO_STANDALONE)

target_link_libraries(papermc_plus_plus PRIVATE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Threading library
find_package(Threads REQUIRED)
target_link_libraries(papermc_plus_plus PRIVATE Threads::Threads)

# Enable Testing
enable_testing()
add_executable(unit_tests tests/test_protocol.cpp src/protocol/buffer.cpp src/world/chunk.cpp)
target_compile_definitions(unit_tests PRIVATE ASIO_STANDALONE)
target_link_libraries(unit_tests PRIVATE spdlog::spdlog Threads::Threads)
add_test(NAME UnitTests COMMAND unit_tests)
add_dependencies(unit_tests generate_registries)
