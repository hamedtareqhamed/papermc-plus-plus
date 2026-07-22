"""
Architect Agent.
Analyzes wiki.vg specifications or user feature requests, breaks them down into C++23 engineering tickets,
and creates structured parallel sub-tasks for concurrent execution.
"""

import sys
import json
import logging
import asyncio
from pathlib import Path
from typing import Dict, Any, List

# Add project root to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from agents.config import generate_completion, async_generate_completion

logger = logging.getLogger("ArchitectAgent")

SYSTEM_PROMPT = """You are an expert Systems Architect specializing in modern C++23 high-performance Minecraft Java Edition Server Engine design.
Your task is to analyze feature requests or protocol specifications and break them down into structured C++ engineering tickets.
Focus on:
1. Data-oriented memory layout and memory safety (std::span, std::expected).
2. Asynchronous non-blocking network I/O with standalone Asio.
3. Thread safety and cache locality.
Format your output as a clear markdown engineering ticket with components, header/source locations, and execution steps.
"""

PARALLEL_DECOMPOSITION_PROMPT = """You are an AI Task Decomposition Engine.
Given a high-level C++ development requirement, decompose it into 3 distinct, non-overlapping sub-tasks for parallel generation:
1. Task A: Header Interfaces & Data Structures (Target: include/core/...)
2. Task B: C++ Source Implementation Logic (Target: src/...)
3. Task C: Verification Suite / Integration Test (Target: tests/...)

Output JSON ONLY in the following format:
{
  "subtasks": [
    {
      "id": "task_a_headers",
      "target_component": "Header Interface & Struct Definitions",
      "preferred_provider": "gemini",
      "instruction": "Detailed prompt for generating headers"
    },
    {
      "id": "task_b_sources",
      "target_component": "Implementation Logic & Codecs",
      "preferred_provider": "groq",
      "instruction": "Detailed prompt for generating source cpp file"
    },
    {
      "id": "task_c_tests",
      "target_component": "Unit Verification & Benchmarks",
      "preferred_provider": "openrouter",
      "instruction": "Detailed prompt for generating test suite"
    }
  ]
}
"""

class ArchitectAgent:
    def __init__(self):
        logger.info("Initializing Architect Agent...")

    def create_plan(self, spec_or_task: str) -> str:
        logger.info(f"Architect Agent processing task: '{spec_or_task}'")
        user_prompt = f"""Task Specification / Feature Request:
{spec_or_task}

Please break down this requirement into an Architectural Execution Plan. 
Specify target file headers, target source files, data structure layouts, and C++23 design patterns to be applied.
"""
        result = generate_completion(prompt=user_prompt, system_instruction=SYSTEM_PROMPT)
        return result

    async def async_create_parallel_tasks(self, spec_or_task: str) -> List[Dict[str, Any]]:
        """Decomposes a high-level task into parallel execution sub-tasks."""
        logger.info(f"[ASYNC] Architect Agent decomposing task into parallel streams: '{spec_or_task}'")
        prompt = f"Task: {spec_or_task}\nDecompose into 3 parallel tasks (Headers, Implementation, Test Suite)."
        
        response_str = await async_generate_completion(prompt=prompt, system_instruction=PARALLEL_DECOMPOSITION_PROMPT)
        
        # Parse JSON output or use structured fallback subtasks
        try:
            # Extract JSON block if enclosed in ```json
            if "```" in response_str:
                json_str = response_str.split("```")[1]
                if json_str.startswith("json"):
                    json_str = json_str[4:]
                data = json.loads(json_str.strip())
            else:
                data = json.loads(response_str.strip())
            return data.get("subtasks", [])
        except Exception as e:
            logger.info(f"Using structured fallback subtasks for parallel execution ({e}).")
            return [
                {
                    "id": "task_a_headers",
                    "target_component": "Header Interface & Struct Definitions",
                    "preferred_provider": "gemini",
                    "instruction": f"Design C++23 header interface for: {spec_or_task}"
                },
                {
                    "id": "task_b_sources",
                    "target_component": "Implementation Logic & Codecs",
                    "preferred_provider": "groq",
                    "instruction": f"Implement C++23 source file logic for: {spec_or_task}"
                },
                {
                    "id": "task_c_tests",
                    "target_component": "Unit Verification Suite",
                    "preferred_provider": "openrouter",
                    "instruction": f"Create test suite for: {spec_or_task}"
                }
            ]

def main():
    agent = ArchitectAgent()
    plan = agent.create_plan("Implement VarInt and VarLong zero-copy codec buffer methods for Minecraft protocol state machine.")
    print("\n--- ARCHITECT EXECUTION PLAN ---")
    print(plan)

if __name__ == "__main__":
    main()
