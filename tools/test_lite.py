import urllib.request
import json

env = {}
with open(".env", "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            k, v = line.split("=", 1)
            env[k.strip()] = v.strip().strip('"').strip("'")

key = env.get("GEMINI_API_KEY")
url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-lite-latest:generateContent?key={key}"

sys_inst = (
    "You are 'Bondhu' (also called 'Kiko'), a very cute animal mascot desktop companion.\n"
    "Speak in Bengali or English. Reply in single valid JSON strictly matching schema:\n"
    '{"speech": "text", "lang": "bn", "emotion": "happy", "display": {"mode": "mascot", "theme": "midnight"}, "smart_home": {"action": "none"}}'
)

body = {
    "system_instruction": {"parts": [{"text": sys_inst}]},
    "contents": [{"role": "user", "parts": [{"text": "Hello Bondhu, kemon acho?"}]}]
}

req = urllib.request.Request(
    url,
    data=json.dumps(body).encode("utf-8"),
    headers={"Content-Type": "application/json"}
)

resp = urllib.request.urlopen(req)
print(resp.read().decode("utf-8"))
