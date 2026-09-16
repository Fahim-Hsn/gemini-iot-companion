#!/usr/bin/env python3
"""
Sprite to RGB565 C Header Converter for Project Bondhu (ST7789 240x240)
Usage:
    python sprite_converter.py input_image.png output_header.h array_name
"""

import sys
import os
try:
    from PIL import Image
except ImportError:
    print("Pillow library not installed. Install with: pip install Pillow")
    sys.exit(1)

def convert_image_to_rgb565(img_path, header_path, array_name="sprite_data"):
    if not os.path.exists(img_path):
        print(f"Error: File {img_path} not found.")
        return False

    img = Image.open(img_path).convert("RGB")
    width, height = img.size
    print(f"Loaded image {img_path} ({width}x{height})")

    if width != 240 or height != 240:
        print(f"Resizing from {width}x{height} to 240x240...")
        img = img.resize((240, 240), Image.Resampling.LANCZOS)
        width, height = 240, 240

    pixels = list(img.getdata())

    with open(header_path, "w", encoding="utf-8") as f:
        f.write("#pragma once\n\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"// Generated sprite: {array_name} ({width}x{height} RGB565)\n")
        f.write(f"const uint16_t {array_name}[{width * height}] PROGMEM = {{\n  ")

        for idx, (r, g, b) in enumerate(pixels):
            # Convert 8-bit RGB to 16-bit RGB565
            r5 = (r >> 3) & 0x1F
            g6 = (g >> 2) & 0x3F
            b5 = (b >> 3) & 0x1F
            rgb565 = (r5 << 11) | (g6 << 5) | b5

            f.write(f"0x{rgb565:04X}, ")
            if (idx + 1) % 12 == 0:
                f.write("\n  ")

        f.write("\n};\n")

    print(f"Successfully generated header: {header_path}")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python sprite_converter.py <input.png> <output.h> <array_name>")
    else:
        convert_image_to_rgb565(sys.argv[1], sys.argv[2], sys.argv[3])
