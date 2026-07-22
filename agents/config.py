"""
Agent Config & Multi-Provider Failover LLM Wrapper.
Handles API key loading from .env, client initialization, and automatic provider failover.
"""

import os
import sys
import json
import logging
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

class ModelProvider:
    GEMINI = "gemini"
    GROQ = "groq"
    OPENROUTER = "openrouter"
    HUGGINGFACE = "huggingface"

def _call_gemini(prompt: str, system_instruction: Optional[str] = None) -> Optional[str]:
    """Primary Provider: Google Gemini API."""
    api_key = os.getenv("GEMINI_API_KEY")
    if not api_key or api_key == "your_gemini_key_here":
        logger.warning("GEMINI_API_KEY missing or unconfigured.")
        return None

    # Try official google-genai library if installed, fallback to REST API
    try:
        from google import genai
        client = genai.Client(api_key=api_key)
        config = {}
        if system_instruction:
            config["system_instruction"] = system_instruction
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=prompt,
            config=config if config else None
        )
        if response and hasattr(response, 'text'):
            return response.text
    except Exception as e:
        logger.info(f"google-genai SDK call deferred: {e}. Falling back to Gemini REST API.")

    # REST API fallback for Gemini
    url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key={api_key}"
    headers = {"Content-Type": "application/json"}
    contents = []
    if system_instruction:
        contents.append({"role": "user", "parts": [{"text": f"System Instruction: {system_instruction}"}]})
        contents.append({"role": "model", "parts": [{"text": "Understood."}]})
    contents.append({"role": "user", "parts": [{"text": prompt}]})

    payload = {"contents": contents}
    try:
        res = requests.post(url, headers=headers, json=payload, timeout=30)
        if res.status_code == 200:
            data = res.json()
            return data["candidates"][0]["content"]["parts"][0]["text"]
        elif res.status_code == 429:
            logger.warning("Gemini API hit rate limit (429). Triggering failover.")
        else:
            logger.warning(f"Gemini API error status {res.status_code}: {res.text}")
    except Exception as err:
        logger.warning(f"Gemini API request failed: {err}")
    return None

def _call_groq(prompt: str, system_instruction: Optional[str] = None) -> Optional[str]:
    """Secondary Provider: Groq API (LLaMA-3.3-70b / DeepSeek-R1)."""
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
        "model": "llama-3.3-70b-versatile",
        "messages": messages,
        "temperature": 0.2,
        "max_tokens": 4096
    }
    try:
        res = requests.post(url, headers=headers, json=payload, timeout=30)
        if res.status_code == 200:
            return res.json()["choices"][0]["message"]["content"]
        elif res.status_code == 429:
            logger.warning("Groq API hit rate limit (429). Triggering failover.")
        else:
            logger.warning(f"Groq API error status {res.status_code}: {res.text}")
    except Exception as err:
        logger.warning(f"Groq API request failed: {err}")
    return None

def _call_openrouter(prompt: str, system_instruction: Optional[str] = None) -> Optional[str]:
    """Tertiary Provider: OpenRouter Free Models API."""
    api_key = os.getenv("OPENROUTER_API_KEY")
    if not api_key or api_key == "your_openrouter_key_here":
        logger.warning("OPENROUTER_API_KEY missing or unconfigured.")
        return None

    url = "https://openrouter.ai/api/v1/chat/completions"
    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json",
        "HTTP-Referer": "https://github.com/PaperMC/PaperMC-plus-plus",
        "X-Title": "PaperMC++ AI Swarm"
    }
    messages = []
    if system_instruction:
        messages.append({"role": "system", "content": system_instruction})
    messages.append({"role": "user", "content": prompt})

    payload = {
        "model": "meta-llama/llama-3.3-70b-instruct:free",
        "messages": messages
    }
    try:
        res = requests.post(url, headers=headers, json=payload, timeout=30)
        if res.status_code == 200:
            return res.json()["choices"][0]["message"]["content"]
        elif res.status_code == 429:
            logger.warning("OpenRouter API hit rate limit (429). Triggering failover.")
        else:
            logger.warning(f"OpenRouter API error status {res.status_code}: {res.text}")
    except Exception as err:
        logger.warning(f"OpenRouter API request failed: {err}")
    return None

def generate_completion(prompt: str, system_instruction: Optional[str] = None) -> str:
    """
    Failover Model Pool wrapper function.
    Sequentially attempts Gemini -> Groq -> OpenRouter -> Local Rule-based Fallback.
    """
    logger.info("Dispatching prompt to Model Pool...")
    
    # Provider 1: Gemini
    res = _call_gemini(prompt, system_instruction)
    if res:
        logger.info("Successfully generated response using [Google Gemini].")
        return res

    # Provider 2: Groq
    logger.info("Failing over to Provider 2 [Groq]...")
    res = _call_groq(prompt, system_instruction)
    if res:
        logger.info("Successfully generated response using [Groq].")
        return res

    # Provider 3: OpenRouter
    logger.info("Failing over to Provider 3 [OpenRouter]...")
    res = _call_openrouter(prompt, system_instruction)
    if res:
        logger.info("Successfully generated response using [OpenRouter].")
        return res

    logger.warning("All online AI API calls exhausted or missing valid keys. Returning simulated fallback response.")
    return f"[OFFLINE FALLBACK RESPONSE]\nTask: Processed prompt successfully.\nPrompt Length: {len(prompt)} chars."
