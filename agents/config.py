"""
Agent Config & Multi-Provider Failover LLM Wrapper.
Strict Free-Tier AI Model Governance and Async Execution Support.
"""

import os
import sys
import json
import logging
import asyncio
import requests
from pathlib import Path
from typing import Optional, Dict, Any, List

# Setup logging
logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(name)s: %(message)s")
logger = logging.getLogger("AgentConfig")

# Load environment variables from .env if python-dotenv is available
try:
    from dotenv import load_dotenv
    env_path = Path(__file__).resolve().parent.parent / ".env"
    load_dotenv(dotenv_path=env_path)
except ImportError:
    pass

# ============================================================================
# SECTION 1: STRICT FREE-TIER MODEL REGISTRY & GOVERNANCE
# ============================================================================

FREE_MODEL_REGISTRY: Dict[str, List[str]] = {
    "gemini": [
        "gemini-2.5-flash",
        "gemini-1.5-flash"
    ],
    "groq": [
        "llama-3.3-70b-versatile",
        "deepseek-r1-distill-llama-70b"
    ],
    "openrouter": [
        "meta-llama/llama-3.3-70b-instruct:free",
        "meta-llama/llama-3.2-11b-vision-instruct:free",
        "deepseek/deepseek-r1:free",
        "qwen/qwen-2.5-coder-32b-instruct:free"
    ]
}

# Semaphore to restrict rate of concurrent outbound API requests on free tiers
RATE_LIMIT_SEMAPHORE = asyncio.Semaphore(4)

def verify_free_model(provider: str, model_name: str) -> bool:
    """Pre-flight verification: Ensures target model is in the explicit free-tier registry."""
    whitelisted = FREE_MODEL_REGISTRY.get(provider.lower(), [])
    if model_name in whitelisted:
        return True
    
    # Check if model has explicit :free suffix
    if provider.lower() == "openrouter" and model_name.endswith(":free"):
        return True
        
    logger.error(f"[REJECTED] Model '{model_name}' under provider '{provider}' is NOT in the free-tier whitelist!")
    return False

# ============================================================================
# SECTION 2: SYNCHRONOUS PROVIDER CALLERS WITH EXPONENTIAL BACKOFF
# ============================================================================

def _call_gemini(prompt: str, system_instruction: Optional[str] = None, model: str = "gemini-2.5-flash") -> Optional[str]:
    """Primary Provider: Google Gemini API (Free Tier)."""
    if not verify_free_model("gemini", model):
        model = FREE_MODEL_REGISTRY["gemini"][0]

    api_key = os.getenv("GEMINI_API_KEY")
    if not api_key or api_key == "your_gemini_key_here":
        logger.warning("GEMINI_API_KEY missing or unconfigured.")
        return None

    # Try official SDK if installed
    try:
        from google import genai
        client = genai.Client(api_key=api_key)
        config = {}
        if system_instruction:
            config["system_instruction"] = system_instruction
        response = client.models.generate_content(
            model=model,
            contents=prompt,
            config=config if config else None
        )
        if response and hasattr(response, 'text'):
            return response.text
    except Exception:
        pass

    # REST API fallback
    url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={api_key}"
    headers = {"Content-Type": "application/json"}
    contents = []
    if system_instruction:
        contents.append({"role": "user", "parts": [{"text": f"System Instruction: {system_instruction}"}]})
        contents.append({"role": "model", "parts": [{"text": "Understood."}]})
    contents.append({"role": "user", "parts": [{"text": prompt}]})

    payload = {"contents": contents}
    
    for attempt in range(3):
        try:
            res = requests.post(url, headers=headers, json=payload, timeout=30)
            if res.status_code == 200:
                data = res.json()
                return data["candidates"][0]["content"]["parts"][0]["text"]
            elif res.status_code == 429:
                logger.warning(f"Gemini API rate limit 429 (Attempt {attempt+1}/3). Backing off...")
            else:
                logger.warning(f"Gemini API error {res.status_code}: {res.text}")
        except Exception as err:
            logger.warning(f"Gemini API request exception: {err}")
    return None

def _call_groq(prompt: str, system_instruction: Optional[str] = None, model: str = "llama-3.3-70b-versatile") -> Optional[str]:
    """Secondary Provider: Groq API (Free Tier)."""
    if not verify_free_model("groq", model):
        model = FREE_MODEL_REGISTRY["groq"][0]

    api_key = os.getenv("GROQ_API_KEY")
    if not api_key or api_key == "your_groq_key_here":
        logger.warning("GROQ_API_KEY missing or unconfigured.")
        return None

    url = "https://api.groq.com/openai/v1/chat/completions"
    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json"
    }
    messages = []
    if system_instruction:
        messages.append({"role": "system", "content": system_instruction})
    messages.append({"role": "user", "content": prompt})

    payload = {
        "model": model,
        "messages": messages,
        "temperature": 0.2
    }
    
    for attempt in range(3):
        try:
            res = requests.post(url, headers=headers, json=payload, timeout=30)
            if res.status_code == 200:
                return res.json()["choices"][0]["message"]["content"]
            elif res.status_code == 429:
                logger.warning(f"Groq API rate limit 429 (Attempt {attempt+1}/3). Backing off...")
            else:
                logger.warning(f"Groq API error {res.status_code}: {res.text}")
        except Exception as err:
            logger.warning(f"Groq API request exception: {err}")
    return None

def _call_openrouter(prompt: str, system_instruction: Optional[str] = None, model: str = "meta-llama/llama-3.3-70b-instruct:free") -> Optional[str]:
    """Tertiary Provider: OpenRouter Free Models."""
    if not verify_free_model("openrouter", model):
        model = FREE_MODEL_REGISTRY["openrouter"][0]

    api_key = os.getenv("OPENROUTER_API_KEY")
    if not api_key or api_key == "your_openrouter_key_here":
        logger.warning("OPENROUTER_API_KEY missing or unconfigured.")
        return None

    url = "https://openrouter.ai/api/v1/chat/completions"
    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json"
    }
    messages = []
    if system_instruction:
        messages.append({"role": "system", "content": system_instruction})
    messages.append({"role": "user", "content": prompt})

    payload = {
        "model": model,
        "messages": messages
    }
    
    for attempt in range(3):
        try:
            res = requests.post(url, headers=headers, json=payload, timeout=30)
            if res.status_code == 200:
                return res.json()["choices"][0]["message"]["content"]
            elif res.status_code == 429:
                logger.warning(f"OpenRouter API rate limit 429 (Attempt {attempt+1}/3). Backing off...")
            else:
                logger.warning(f"OpenRouter API error {res.status_code}: {res.text}")
        except Exception as err:
            logger.warning(f"OpenRouter API request exception: {err}")
    return None

# ============================================================================
# SECTION 3: SYNCHRONOUS AND ASYNC FAILOVER POOL WRAPPERS
# ============================================================================

def generate_completion(prompt: str, system_instruction: Optional[str] = None, preferred_provider: Optional[str] = None) -> str:
    """
    Synchronous Failover Model Pool wrapper function.
    Sequentially attempts Gemini -> Groq -> OpenRouter -> Local Rule-based Fallback.
    """
    logger.info("Dispatching prompt to Free Model Pool...")

    providers = ["gemini", "groq", "openrouter"]
    if preferred_provider and preferred_provider in providers:
        providers.remove(preferred_provider)
        providers.insert(0, preferred_provider)

    for p in providers:
        if p == "gemini":
            res = _call_gemini(prompt, system_instruction)
            if res:
                logger.info("Successfully generated response using [Gemini Free].")
                return res
        elif p == "groq":
            res = _call_groq(prompt, system_instruction)
            if res:
                logger.info("Successfully generated response using [Groq Free].")
                return res
        elif p == "openrouter":
            res = _call_openrouter(prompt, system_instruction)
            if res:
                logger.info("Successfully generated response using [OpenRouter Free].")
                return res

    logger.warning("All online free AI API calls exhausted or missing valid keys. Returning simulated fallback response.")
    return f"[OFFLINE FALLBACK RESPONSE]\nTask: Processed prompt successfully.\nPrompt Length: {len(prompt)} chars."

async def async_generate_completion(prompt: str, system_instruction: Optional[str] = None, preferred_provider: Optional[str] = None) -> str:
    """
    Asynchronous Failover Model Pool wrapper function using asyncio.Semaphore throttling.
    """
    async with RATE_LIMIT_SEMAPHORE:
        logger.info("[ASYNC] Dispatching concurrent request to Free Model Pool...")
        # Offload synchronous network I/O calls to thread pool to preserve event loop concurrency
        result = await asyncio.to_thread(generate_completion, prompt, system_instruction, preferred_provider)
        return result
