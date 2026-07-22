"""
Parallel Agent Runner Engine.
Coordinates concurrent execution of multiple CoderAgent instances across free model providers using asyncio.gather().
"""

import sys
import time
import logging
import asyncio
from pathlib import Path
from typing import List, Dict, Any

# Add project root to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from agents.architect_agent import ArchitectAgent
from agents.coder_agent import CoderAgent
from agents.git_agent import GitAgent

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] ParallelRunner: %(message)s")
logger = logging.getLogger("ParallelRunner")

class ParallelAgentRunner:
    def __init__(self):
        self.architect = ArchitectAgent()
        self.coder = CoderAgent()
        self.git_agent = GitAgent()

    async def execute_task_worker(self, task_info: Dict[str, Any]) -> Dict[str, Any]:
        """Worker task launched concurrently in asyncio event loop."""
        task_id = task_info.get("id", "task_unknown")
        component = task_info.get("target_component", "Core Component")
        provider = task_info.get("preferred_provider", "gemini")
        instruction = task_info.get("instruction", "Generate C++23 code")

        logger.info(f"[WORKER-{task_id}] Launching parallel agent on provider [{provider}] for target: {component}")
        start_time = time.time()
        
        code_output = await self.coder.async_generate_cpp_code(
            architecture_plan=instruction,
            target_component=component,
            preferred_provider=provider
        )
        elapsed = time.time() - start_time
        logger.info(f"[WORKER-{task_id}] Completed in {elapsed:.2f}s using provider [{provider}].")

        return {
            "id": task_id,
            "component": component,
            "provider": provider,
            "elapsed_seconds": elapsed,
            "code": code_output
        }

    async def run_parallel_pipeline(self, main_task: str, auto_commit: bool = True) -> Dict[str, Any]:
        """Main async entrypoint for parallel task execution."""
        logger.info("==================================================")
        logger.info("STARTING ASYNC PARALLEL MULTI-AGENT SWARM ENGINE")
        logger.info(f"Main Task: {main_task}")
        logger.info("==================================================")

        total_start = time.time()

        # Step 1: Decompose Task into Parallel Streams
        logger.info("\n[STAGE 1] Decomposing task into independent parallel sub-tasks...")
        subtasks = await self.architect.async_create_parallel_tasks(main_task)
        logger.info(f"Decomposed into {len(subtasks)} concurrent streams.")

        # Step 2: Concurrent Execution via asyncio.gather
        logger.info("\n[STAGE 2] Dispatching workers concurrently across Free Model Pool...")
        worker_coroutines = [self.execute_task_worker(task) for task in subtasks]
        results = await asyncio.gather(*worker_coroutines)

        total_elapsed = time.time() - total_start

        logger.info("\n==================================================")
        logger.info("PARALLEL WORKER EXECUTION METRICS")
        logger.info("==================================================")
        merged_code_blocks = []
        for res in results:
            logger.info(f" -> Subtask [{res['id']}]: Component='{res['component']}', Provider='{res['provider']}', Time={res['elapsed_seconds']:.2f}s")
            merged_code_blocks.append(f"// === SUBTASK: {res['component']} (Provider: {res['provider']}) ===\n" + res['code'])

        full_merged_output = "\n\n".join(merged_code_blocks)

        # Step 3: Git Pipeline Integration
        if auto_commit:
            logger.info("\n[STAGE 3] Invoking Git Agent for formatting, staging, and commit...")
            commit_success = self.git_agent.commit(f"parallel: {main_task}")
            if commit_success:
                logger.info("[SUCCESS] Parallel swarm execution committed cleanly to Git!")
            else:
                logger.warning("[WARN] Code generated but Git commit encountered warnings.")

        logger.info(f"\n[COMPLETE] Total Parallel Pipeline Duration: {total_elapsed:.2f} seconds.")
        return {
            "total_elapsed": total_elapsed,
            "worker_results": results,
            "merged_output": full_merged_output
        }

def main():
    runner = ParallelAgentRunner()
    task = "Implement Minecraft Protocol Packet Serialization and VarInt Codecs"
    asyncio.run(runner.run_parallel_pipeline(main_task=task, auto_commit=False))

if __name__ == "__main__":
    main()
