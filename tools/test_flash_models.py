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

test_models = [
    "gemini-flash-latest",
    "gemini-flash-lite-latest",
    "gemini-3.6-flash",
    "gemini-3.5-flash",
    "gemini-3.1-flash-lite",
    "gemini-3.7-flash",
    "gemini-3.8-flash",
]

for model in test_models:
    url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={key}"
    req = urllib.request.Request(
        url,
        data=json.dumps({"contents": [{"role": "user", "parts": [{"text": "Hello"}]}]}).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    try:
        resp = urllib.request.urlopen(req, timeout=10)
        res_data = json.loads(resp.read())
        ans = res_data["candidates"][0]["content"]["parts"][0]["text"].strip()
        print(f"[SUCCESS 200] {model} -> {ans[:50]}")
    except urllib.error.HTTPError as e:
        err_body = e.read().decode("utf-8", errors="ignore")
        print(f"[FAIL {e.code}] {model} -> {err_body[:100]}")
    except Exception as e:
        print(f"[ERR] {model} -> {e}")
