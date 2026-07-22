# PaperMC++ Server Engine (v0.1.0-alpha.1)

An enterprise-grade, ultra-high-performance Minecraft Java Edition Server Engine core built from scratch in modern **C++23**, coupled with an automated **Python Multi-Agent AI Swarm System** for continuous feature development and protocol implementation.

> **Design Core:** C++23, Zero-Copy Protocol Serialization, Lock-Free Async Networking (Asio), Data-Oriented ECS Chunk Architecture, and Embedded LuaJIT Scripting support.

---

## 🚀 Key Features in v0.1.0-alpha.1

* **Modern C++23 Core:** Standard C++23 (`-std=c++23`) with strict memory safety (`std::span`, `std::unique_ptr`, non-owning views), zero raw `new`/`delete`, and monadic error handling via `std::expected`.
* **Asynchronous Non-Blocking Networking:** Powered by standalone `asio` for high throughput networking capable of handling thousands of concurrent client packet loops.
* **Data-Oriented Chunk Memory Layout:** Dynamic palette-based 16x16x16 chunk section storage (`alignas(64)`) optimized for L1/L2 cache line efficiency and zero-copy packet broadcasts.
* **Procedural World Generation:** Built-in C++23 Flatland and Simplex Noise heightmap generation.
* **LuaJIT Scripting API:** Native plugin engine powered by `sol2` with event bindings (`onPlayerJoin`, `onBlockBreak`, `onChatMessage`).
* **Autonomous AI Development Swarm (`agents/`):** Async multi-agent runner featuring automatic failover between free AI providers (Google Gemini 2.5 Flash, Groq LLaMA 3.3 70B / DeepSeek R1, OpenRouter / HuggingFace).

---

## 🛠️ Project Structure

```plaintext
.
├── CMakeLists.txt              # Standard C++23 CMake configuration (v0.1.0)
├── .env.example                # API key template for Python AI agents
├── README.md                   # Project overview & guide
├── ARCHITECTURE.md             # In-depth architectural design blueprint
├── RELEASE_NOTES.md            # Release notes for v0.1.0-alpha.1
├── include/core/
│   ├── network/                # Async socket listeners and connection session handlers
│   ├── protocol/               # Minecraft VarInt, ByteBuf, Packet definitions & ciphers
│   ├── world/                  # Cache-friendly block palette, chunk sections, and entities
│   └── scripting/              # LuaJIT / sol2 plugin engine & event bus
├── src/                        # Implementation files for core engine subsystems
├── plugins/                    # Lua plugin scripts (example.lua)
├── tests/                      # C++ unit test suite
├── docs/                       # wiki.vg protocol specifications & PaperMC references
└── agents/                     # Async Multi-agent Python AI swarm (Parallel Runner)
```

---

## 💻 Building and Running

### Prerequisites
* C++23 compliant compiler (`GCC 13+`, `Clang 16+`)
* `CMake 3.25+`
* `Ninja` or `Make`
* Python `3.10+` with `requests` and `python-dotenv`

### C++ Server Compilation
```bash
# Configure and Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Execute Unit Tests
ctest --test-dir build --output-on-failure

# Launch Engine Binary
./build/papermc_plus_plus
```

### Running the Python AI Agent Swarm Engine
```bash
# Run Async Parallel Swarm Engine
python3 agents/orchestrator.py --parallel --task "Implement Minecraft Packet Serialization"
```

