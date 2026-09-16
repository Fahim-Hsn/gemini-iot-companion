#!/usr/bin/env python3
"""
Project Bondhu - Local API Simulator & Tester
Test your Google Gemini Pro/Flash API Key & Google TTS credentials from PC.
Usage:
    python simulate_api.py
"""

import json
import os
import re
import urllib.request
import urllib.error

# Load from .env if available
def load_env_file(filepath=".env"):
    env_vars = {}
    if os.path.exists(filepath):
        with open(filepath, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                match = re.match(r"^([A-Za-z0-9_]+)\s*=\s*[\"']?(.*?)[\"']?$", line)
                if match:
                    k, v = match.groups()
                    env_vars[k] = v
    return env_vars

_env = load_env_file()
GEMINI_API_KEY = _env.get("GEMINI_API_KEY", "YOUR_GEMINI_API_KEY")
GEMINI_MODEL = "gemini-3.6-flash"

SYSTEM_INSTRUCTION = """
You are 'Bondhu' (also called 'Kiko'), a very cute, helpful, and lively animal mascot (Kitsune/Fox) desktop AI companion created for Fahim.
Guidelines:
1. You speak both Bengali (Bangla) and English. If the user speaks Bangla, reply in cute Bangla. If in English, reply in cheerful English.
2. Keep your speech concise (1 to 2 sentences max) so it sounds natural on a desktop speaker.
3. ALWAYS format your entire response as a valid, single JSON object strictly matching this schema:
{
  "speech": "Your voice reply text",
  "lang": "bn" or "en",
  "emotion": "idle" | "happy" | "sad" | "confused" | "thinking" | "excited" | "sleeping",
  "display": {
    "mode": "mascot" | "clock" | "weather" | "todo" | "card",
    "title": "optional title",
    "subtitle": "optional subtitle",
    "theme": "midnight" | "cyberpunk" | "pastel" | "green"
  },
  "smart_home": {
    "action": "none" | "light_on" | "light_off" | "fan_on" | "fan_off" | "set_alarm" | "set_timer",
    "target": "light" | "fan" | "alarm",
    "value": 0,
    "meta": "description"
  }
}
Do not include markdown wrappers (like ```json), just output the raw JSON string.
"""

def test_gemini(prompt):
    url = f"https://generativelanguage.googleapis.com/v1beta/models/{GEMINI_MODEL}:generateContent?key={GEMINI_API_KEY}"
    
    payload = {
        "system_instruction": {
            "parts": [{"text": SYSTEM_INSTRUCTION}]
        },
        "contents": [
            {
                "role": "user",
                "parts": [{"text": prompt}]
            }
        ]
    }

    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )

    try:
        print(f"Sending prompt to Gemini: '{prompt}'...")
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            raw_text = data["candidates"][0]["content"]["parts"][0]["text"].strip()
            print("\n--- Raw Response from Gemini ---")
            print(raw_text)

            # Clean markdown code block if present
            clean_text = raw_text
            if clean_text.startswith("```json"):
                clean_text = clean_text[7:]
            if clean_text.startswith("```"):
                clean_text = clean_text[3:]
            if clean_text.endswith("```"):
                clean_text = clean_text[:-3]
            clean_text = clean_text.strip()

            parsed = json.loads(clean_text)
            print("\n--- Parsed Structured Response ---")
            print(f"Speech:   {parsed.get('speech')}")
            print(f"Language: {parsed.get('lang')}")
            print(f"Emotion:  {parsed.get('emotion')}")
            print(f"Display:  {parsed.get('display')}")
            print(f"Home:     {parsed.get('smart_home')}")

    except urllib.error.HTTPError as e:
        print(f"HTTP Error {e.code}: {e.read().decode('utf-8')}")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    test_prompts = [
        "হ্যালো বন্ধু! কেমন আছো? আজ ঢাকা শহরের আবহাওয়া কেমন?",
        "Bondhu, turn on the bedroom lights and show me the clock.",
        "আমাকে একটা সুন্দর জোক শোনাও তো!"
    ]

    print("Project Bondhu API Simulator")
    print("---------------------------------------------")
    if GEMINI_API_KEY == "YOUR_GEMINI_API_KEY":
        print("Please edit simulate_api.py and set your GEMINI_API_KEY first.")
    else:
        for p in test_prompts:
            print("=" * 50)
            test_gemini(p)
