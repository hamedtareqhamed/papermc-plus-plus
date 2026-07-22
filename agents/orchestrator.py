"""
Multi-Agent Orchestrator Script.
Main entrypoint script that ties ArchitectAgent, CoderAgent, GitAgent, and ParallelAgentRunner together.
Supports both Sequential and Async Parallel execution modes.
"""

import sys
import argparse
import logging
import asyncio
from pathlib import Path

# Add project root to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from agents.architect_agent import ArchitectAgent
from agents.coder_agent import CoderAgent
from agents.git_agent import GitAgent
from agents.parallel_runner import ParallelAgentRunner

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] Orchestrator: %(message)s")
logger = logging.getLogger("Orchestrator")

class AgentSwarmOrchestrator:
    def __init__(self):
        self.architect = ArchitectAgent()
        self.coder = CoderAgent()
        self.git_agent = GitAgent()
        self.parallel_runner = ParallelAgentRunner()

    def run_sequential_pipeline(self, task_description: str, auto_commit: bool = False):
        logger.info("==================================================")
        logger.info(f"STARTING SEQUENTIAL MULTI-AGENT SWARM PIPELINE")
        logger.info(f"Task: {task_description}")
        logger.info("==================================================")

        # Step 1: Architect Plan
        logger.info("\n[STAGE 1] Invoking Architect Agent for plan breakdown...")
        plan = self.architect.create_plan(task_description)
        logger.info("Architect Execution Plan created successfully.")
        print("\n--- ARCHITECT EXECUTION PLAN ---")
        print(plan)

        # Step 2: Coder Implementation
        logger.info("\n[STAGE 2] Invoking Coder Agent to synthesize C++23 code...")
        code = self.coder.generate_cpp_code(architecture_plan=plan, target_component="Core Module")
        logger.info("Coder Agent synthesis finished.")
        print("\n--- SYNTHESIZED CODE SNIPPET ---")
        print(code[:500] + "\n..." if len(code) > 500 else code)

        # Step 3: Git Agent Automation
        if auto_commit:
            logger.info("\n[STAGE 3] Invoking Git Agent for formatting and commit...")
            success = self.git_agent.commit(task_description)
            if success:
                logger.info("Pipeline executed and committed successfully!")
            else:
                logger.warning("Pipeline executed but git commit failed.")
        else:
            logger.info("\n[STAGE 3] Skipping git commit (auto_commit=False).")

        logger.info("==================================================")
        logger.info("SEQUENTIAL SWARM PIPELINE COMPLETE")
        logger.info("==================================================")

    def run_parallel_pipeline(self, task_description: str, auto_commit: bool = False):
        logger.info("==================================================")
        logger.info(f"STARTING ASYNC PARALLEL MULTI-AGENT SWARM PIPELINE")
        logger.info(f"Task: {task_description}")
        logger.info("==================================================")
        
        asyncio.run(self.parallel_runner.run_parallel_pipeline(main_task=task_description, auto_commit=auto_commit))

def main():
    parser = argparse.ArgumentParser(description="PaperMC++ AI Multi-Agent Swarm Orchestrator")
    parser.add_argument("--task", type=str, default="Implement Minecraft Packet Serialization and VarInt Codecs",
                        help="Development task description to run through agent swarm.")
    parser.add_argument("--parallel", action="store_true", help="Enable async parallel agent execution engine.")
    parser.add_argument("--commit", action="store_true", help="Automatically commit changes on success.")

    args = parser.parse_args()
    orchestrator = AgentSwarmOrchestrator()

    if args.parallel:
        orchestrator.run_parallel_pipeline(task_description=args.task, auto_commit=args.commit)
    else:
        orchestrator.run_sequential_pipeline(task_description=args.task, auto_commit=args.commit)

if __name__ == "__main__":
    main()
