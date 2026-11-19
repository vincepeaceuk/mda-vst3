#!/usr/bin/env python3
"""Fetch and verify the exact JUCE source used for this preview."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
JUCE = ROOT / "deps/JUCE"
COMMIT = "d60b7a78030661574a615b586d9325ba7a7fa7dd"
if not JUCE.exists():
    JUCE.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["git", "clone", "--depth", "1", "--branch", "9.0.5",
                    "https://github.com/juce-framework/JUCE.git", str(JUCE)], check=True)
if (JUCE / ".git").exists():
    actual = subprocess.check_output(["git", "-C", str(JUCE), "rev-parse", "HEAD"], text=True).strip()
else:
    marker = JUCE / "MDA-PINNED-COMMIT.txt"
    actual = marker.read_text().strip() if marker.exists() else "unverified"
if actual != COMMIT:
    raise SystemExit(f"JUCE revision differs: expected {COMMIT}, found {actual}; existing files were not changed")
print(f"JUCE 9.0.5 verified: {COMMIT}")
