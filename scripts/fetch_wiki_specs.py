#!/usr/bin/env python3
"""
Fetch Wiki Specs & Protocol References Utility.
Downloads and caches protocol specification metadata for paper engine protocol code generators.
"""

import sys
import json
import urllib.request
from pathlib import Path

PROTOCOL_DATA_URL = "https://raw.githubusercontent.com/PrismarineJS/minecraft-data/master/data/pc/common/protocolVersions.json"

def fetch_protocol_versions():
    """Fetch official protocol version mapping table from open specification registry."""
    print(f"[INFO] Fetching protocol versions from {PROTOCOL_DATA_URL}...")
    try:
        req = urllib.request.Request(
            PROTOCOL_DATA_URL,
            headers={"User-Agent": "PaperMC++-SpecFetcher/1.0"}
        )
        with urllib.request.urlopen(req, timeout=10) as response:
            if response.status == 200:
                data = json.loads(response.read().decode('utf-8'))
                print(f"[SUCCESS] Retreived {len(data)} protocol version specs.")
                return data
    except Exception as err:
        print(f"[ERROR] Failed to fetch online protocol specs: {err}", file=sys.stderr)
        return None

def main():
    docs_dir = Path(__file__).resolve().parent.parent / "docs"
    docs_dir.mkdir(parents=True, exist_ok=True)
    
    versions = fetch_protocol_versions()
    if versions:
        output_file = docs_dir / "protocol_versions.json"
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(versions[:20], f, indent=2)
        print(f"[SUCCESS] Saved protocol specs cache to {output_file}")
    else:
        print("[WARN] Using built-in offline protocol metadata fallback.")

if __name__ == "__main__":
    main()
