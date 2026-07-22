"""
Coder Agent.
Generates high-performance modern C++ code following strict C++23 guidelines:
- Zero raw pointers, use std::unique_ptr, std::shared_ptr, std::span, std::string_view.
- Exception-free monadic error handling using std::expected or std::optional.
- High-throughput zero-copy serialization and cache-friendly data alignment.
"""

import sys
import logging
import asyncio
from typing import Optional
from pathlib import Path

# Add project root to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from agents.config import generate_completion, async_generate_completion

logger = logging.getLogger("CoderAgent")

SYSTEM_PROMPT = """You are a Senior C++23 Systems Developer building an ultra-fast Minecraft Java Edition Server Engine core.
Rules for generated C++ code:
1. Standard: C++23 (-std=c++23).
2. Memory Safety: Use std::unique_ptr, std::shared_ptr, std::span, std::string_view. NEVER use raw new/delete or unmanaged raw pointers.
3. Monadic Error Handling: Use std::expected<T, E> or std::optional<T> instead of throwing exceptions.
4. Cache Line Alignment: Use Data-Oriented Design (DOD) with explicit structure packing and alignment (alignas(64) where applicable).
5. Concurrency: Use modern C++23 threading models (std::jthread, std::stop_token, std::atomic, lock-free queues).
Provide clean, idiomatic header (.hpp) and source (.cpp) implementations without placeholders.
"""

class CoderAgent:
    def __init__(self):
        logger.info("Initializing Coder Agent...")

    def generate_cpp_code(self, architecture_plan: str, target_component: str, preferred_provider: Optional[str] = None) -> str:
        logger.info(f"Coder Agent generating code for target component: '{target_component}'")
        user_prompt = f"""Architecture Plan:
{architecture_plan}

Target Component:
{target_component}

Generate standard C++23 header and source file contents adhering strictly to our modern C++23 guidelines.
"""
        code = generate_completion(prompt=user_prompt, system_instruction=SYSTEM_PROMPT, preferred_provider=preferred_provider)
        return code

    async def async_generate_cpp_code(self, architecture_plan: str, target_component: str, preferred_provider: Optional[str] = None) -> str:
        """Asynchronous generation method for parallel worker dispatch."""
        logger.info(f"[ASYNC] Coder Agent generating code concurrently for '{target_component}' via provider '{preferred_provider}'...")
        user_prompt = f"""Architecture Plan:
{architecture_plan}

Target Component:
{target_component}

Generate standard C++23 header/source code adhering strictly to modern C++23 guidelines.
"""
        code = await async_generate_completion(prompt=user_prompt, system_instruction=SYSTEM_PROMPT, preferred_provider=preferred_provider)
        return code

def main():
    agent = CoderAgent()
    code = agent.generate_cpp_code(
        architecture_plan="Create a thread-safe VarInt decoder using std::span<const std::byte>.",
        target_component="protocol::VarInt"
    )
    print("\n--- GENERATED C++ CODE ---")
    print(code)

if __name__ == "__main__":
    main()
