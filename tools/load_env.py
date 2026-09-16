import os
import re

Import("env")

# Fix for Windows PlatformIO RISC-V toolchain PATH
toolchain_base = os.path.expanduser(r"~/.platformio/packages/toolchain-riscv32-esp")
tc_bin = os.path.join(toolchain_base, "bin")
tc_sub_bin = os.path.join(toolchain_base, "riscv32-esp-elf", "bin")

if os.path.exists(tc_bin):
    env.PrependENVPath("PATH", tc_bin)
    os.environ["PATH"] = tc_bin + os.pathsep + os.environ.get("PATH", "")
if os.path.exists(tc_sub_bin):
    env.PrependENVPath("PATH", tc_sub_bin)
    os.environ["PATH"] = tc_sub_bin + os.pathsep + os.environ.get("PATH", "")

# Path to .env and target secrets.h
env_path = os.path.join(env.get("PROJECT_DIR"), ".env")
secrets_header = os.path.join(env.get("PROJECT_DIR"), "include", "secrets.h")

if os.path.exists(env_path):
    print(f"[Bondhu Build] Found .env file! Generating include/secrets.h securely...")
    env_vars = {}
    with open(env_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            match = re.match(r"^([A-Za-z0-9_]+)\s*=\s*[\"']?(.*?)[\"']?$", line)
            if match:
                key, val = match.groups()
                env_vars[key] = val

    # Generate secrets.h
    with open(secrets_header, "w", encoding="utf-8") as f:
        f.write("// AUTO-GENERATED FROM .env — DO NOT COMMIT TO GIT\n")
        f.write("#pragma once\n\n")
        for k, v in env_vars.items():
            f.write(f'#define {k} "{v}"\n')

    print("[Bondhu Build] include/secrets.h generated successfully from .env.")
else:
    print("[Bondhu Build] Note: No .env file found. Using defaults or manual include/secrets.h.")
