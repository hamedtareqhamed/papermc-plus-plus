"""
Multi-Agent Orchestrator Script.
Main entrypoint script that ties ArchitectAgent, CoderAgent, and GitAgent together
to execute end-to-end development pipeline tasks.
"""

import sys
import argparse
import logging
from pathlib import Path

# Add project root to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from agents.architect_agent import ArchitectAgent
from agents.coder_agent import CoderAgent
from agents.git_agent import GitAgent

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] Orchestrator: %(message)s")
logger = logging.getLogger("Orchestrator")

class AgentSwarmOrchestrator:
    def __init__(self):
        self.architect = ArchitectAgent()
        self.coder = CoderAgent()
        self.git_agent = GitAgent()

    def run_pipeline(self, task_description: str, auto_commit: bool = False):
        logger.info("==================================================")
        logger.info(f"STARTING MULTI-AGENT SWARM PIPELINE")
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
        logger.info("SWARM PIPELINE COMPLETE")
        logger.info("==================================================")

def main():
    parser = argparse.ArgumentParser(description="PaperMC++ AI Multi-Agent Swarm Orchestrator")
    parser.add_argument("--task", type=str, default="Setup foundational C++23 Minecraft server engine protocol core.",
                        help="Development task description to run through agent swarm.")
    parser.add_argument("--commit", action="store_true", help="Automatically commit changes on success.")

    args = parser.parse_args()
    orchestrator = AgentSwarmOrchestrator()
    orchestrator.run_pipeline(task_description=args.task, auto_commit=args.commit)

if __name__ == "__main__":
    main()
