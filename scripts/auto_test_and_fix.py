#!/usr/bin/env python3
"""
Autonomous Repair Loop Wrapper Script.
Orchestrates: Run C++ Server -> Launch Node.js Test Bot -> Capture Errors -> Invoke AI Coder Repair Agent -> Recompile & Retest.
"""

import sys
import time
import subprocess
import signal
import os
import logging
from pathlib import Path

# Add project root to sys.path
PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT))

from agents.coder_agent import CoderAgent
from agents.git_agent import GitAgent

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] AutoRepairLoop: %(message)s")
logger = logging.getLogger("AutoRepairLoop")

def kill_previous_instances():
    """Kill any hanging server instances on port 25565."""
    try:
        subprocess.run(["pkill", "-9", "-f", "papermc_plus_plus"], check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.5)
    except Exception:
        pass

def compile_server() -> bool:
    """Build C++ server using CMake."""
    logger.info("Compiling C++ PaperMC++ engine binary via CMake...")
    try:
        res = subprocess.run(["cmake", "--build", "build", "-j4"], cwd=PROJECT_ROOT, capture_output=True, text=True)
        if res.returncode == 0:
            logger.info("C++ Compilation succeeded cleanly (0 errors).")
            return True
        else:
            logger.error(f"C++ Compilation failed:\n{res.stderr}")
            return False
    except Exception as e:
        logger.error(f"Failed executing cmake build: {e}")
        return False

def run_integration_test(max_attempts: int = 3) -> bool:
    coder = CoderAgent()
    git_agent = GitAgent()

    for attempt in range(1, max_attempts + 1):
        logger.info(f"\n==================================================")
        logger.info(f"AUTONOMOUS REPAIR LOOP - ATTEMPT {attempt}/{max_attempts}")
        logger.info(f"==================================================")

        kill_previous_instances()

        # Step 1: Compile server
        if not compile_server():
            logger.error("Compilation step failed before launching bot.")
            continue

        # Step 2: Spawn Server Process in background
        server_bin = PROJECT_ROOT / "build" / "papermc_plus_plus"
        if not server_bin.exists():
            logger.error(f"Server binary missing at {server_bin}")
            return False

        logger.info(f"Spawning PaperMC++ Server binary in background: {server_bin}")
        server_process = subprocess.Popen(
            [str(server_bin)],
            cwd=PROJECT_ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        # Wait 2 seconds for server socket listening
        time.sleep(2)

        # Check if server crashed immediately
        if server_process.poll() is not None:
            stdout, stderr = server_process.communicate()
            logger.error(f"Server process terminated prematurely!\nStdout: {stdout}\nStderr: {stderr}")
            continue

        # Step 3: Run Node.js Bot Test Harness
        logger.info("Launching Node.js mineflayer bot integration harness (scripts/test_bot.js)...")
        bot_process = subprocess.run(
            ["node", "scripts/test_bot.js"],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=35
        )

        logger.info(f"Bot Process Exit Code: {bot_process.returncode}")
        print("\n--- BOT TEST STDOUT ---")
        print(bot_process.stdout)

        if bot_process.returncode != 0:
            print("\n--- BOT TEST STDERR ---")
            print(bot_process.stderr)

        # Step 4: Evaluate Test Result
        if bot_process.returncode == 0:
            logger.info("==================================================")
            logger.info("🎉 INTEGRATION TEST PASSED! BOT CONNECTED & STABLE")
            logger.info("==================================================")
            
            # Clean shutdown of server process
            server_process.send_signal(signal.SIGINT)
            try:
                server_process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                server_process.kill()
            kill_previous_instances()
            return True
        else:
            logger.warning(f"[ATTEMPT {attempt}] Integration test failed. Initiating AI Coder repair step...")
            
            # Capture server and bot output logs for diagnosis
            server_process.send_signal(signal.SIGINT)
            try:
                server_stdout, server_stderr = server_process.communicate(timeout=3)
            except Exception:
                server_process.kill()
                server_stdout, server_stderr = "", ""

            error_context = f"""Server Stdout:
{server_stdout}
Server Stderr:
{server_stderr}
Bot Test Failure Output:
{bot_process.stdout}
{bot_process.stderr}
"""
            logger.info("Dispatching failure logs to Coder Agent for automated diagnosis and code repair...")
            repair_code = coder.generate_cpp_code(
                architecture_plan=f"Fix C++ Minecraft protocol serialization bug causing bot error:\n{error_context}",
                target_component="protocol::PacketSerializer"
            )
            logger.info("Coder Agent generated repair candidate.")
            print("\n--- AI CODER REPAIR PROPOSED SOLUTION ---")
            print(repair_code[:400] + "\n..." if len(repair_code) > 400 else repair_code)

    kill_previous_instances()
    logger.error("Autonomous Repair Loop exhausted maximum attempts without resolution.")
    return False

def main():
    success = run_integration_test(max_attempts=3)
    if success:
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
