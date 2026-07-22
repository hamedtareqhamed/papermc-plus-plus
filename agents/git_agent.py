"""
Git Agent.
Handles git repository automation: running clang-format (if available), staging files,
and crafting semantic git commits based on engineering tickets or file changes.
"""

import sys
import subprocess
import shutil
import logging
from pathlib import Path

# Add project root to sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from agents.config import generate_completion

logger = logging.getLogger("GitAgent")

class GitAgent:
    def __init__(self, repo_path: Optional[Path] = None):
        self.repo_path = repo_path or Path(__file__).resolve().parent.parent
        logger.info(f"Initializing Git Agent at repository root: {self.repo_path}")

    def format_code(self) -> bool:
        """Run clang-format on all C++ headers and source files if clang-format is installed."""
        clang_format = shutil.which("clang-format")
        if not clang_format:
            logger.info("clang-format not found in system PATH. Skipping auto-formatting.")
            return True

        logger.info("Running clang-format on C++ codebase...")
        cpp_files = list(self.repo_path.glob("include/**/*.[h|hpp]")) + list(self.repo_path.glob("src/**/*.[c|cpp]"))
        for file in cpp_files:
            try:
                subprocess.run([clang_format, "-i", str(file)], check=True)
            except subprocess.CalledProcessError as e:
                logger.error(f"Failed to format file {file}: {e}")
                return False
        logger.info("Code formatting complete.")
        return True

    def stage_all(self) -> bool:
        """Stage all modified and untracked files."""
        try:
            subprocess.run(["git", "add", "."], cwd=self.repo_path, check=True)
            logger.info("Staged all workspace changes via 'git add .'")
            return True
        except subprocess.CalledProcessError as e:
            logger.error(f"Git add failed: {e}")
            return False

    def generate_commit_message(self, task_summary: str) -> str:
        """Generate a semantic commit message using LLM or structured rules."""
        prompt = f"""Generate a concise semantic git commit header and body for the following change summary:
Task: {task_summary}

Requirements:
- Conventional Commit format: <type>(<scope>): <short summary>
- Valid types: feat, fix, refactor, docs, chore, perf
- Maximum 72 characters for title line
- Optional body explaining rationale
Return ONLY the final commit message string.
"""
        message = generate_completion(prompt=prompt).strip()
        if not message or "[OFFLINE FALLBACK RESPONSE]" in message:
            # Rule-based fallback
            return f"feat(core): {task_summary.lower()}"
        return message

    def commit(self, task_summary: str) -> bool:
        """Stage changes and commit with semantic message."""
        self.format_code()
        if not self.stage_all():
            return False

        commit_msg = self.generate_commit_message(task_summary)
        logger.info(f"Committing with message:\n{commit_msg}")
        try:
            subprocess.run(["git", "commit", "-m", commit_msg], cwd=self.repo_path, check=True)
            logger.info("Git commit successfully created.")
            return True
        except subprocess.CalledProcessError as e:
            logger.error(f"Git commit failed: {e}")
            return False

def main():
    agent = GitAgent()
    agent.format_code()
    print("Git agent initialized successfully.")

if __name__ == "__main__":
    main()
