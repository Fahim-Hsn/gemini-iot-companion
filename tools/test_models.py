import urllib.request
import json
import os

env = {}
with open(".env", "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            k, v = line.split("=", 1)
            env[k.strip()] = v.strip().strip('"').strip("'")

key = env.get("GEMINI_API_KEY")
print(f"Testing with API Key: {key[:8]}...{key[-6:]}")

models = ["gemini-2.5-flash", "gemini-2.0-flash", "gemini-1.5-flash", "gemini-1.5-pro", "gemini-2.5-flash-lite"]

for model in models:
    url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={key}"
    req = urllib.request.Request(
        url,
        data=json.dumps({"contents": [{"role": "user", "parts": [{"text": "say hello"}]}]}).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    try:
        resp = urllib.request.urlopen(req, timeout=10)
        print(f"[OK] {model}: HTTP {resp.status}")
    except urllib.error.HTTPError as e:
        err_body = e.read().decode("utf-8", errors="ignore")
        print(f"[FAIL] {model}: HTTP {e.code} - {err_body[:120]}")
    except Exception as e:
        print(f"[ERR] {model}: {e}")
