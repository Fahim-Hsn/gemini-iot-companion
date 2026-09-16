# 🦊 Project Bondhu (বন্ধু) — AI Desktop Companion

> **A smart, expressive AI desktop mascot powered by Google Gemini Pro / Flash 2.0, ESP32-C6, ST7789 IPS Display, INMP441 Microphone, and MAX98357A I2S DAC Audio.**

---

## 🌟 Overview (প্রজেক্ট পরিচিতি)

**Project Bondhu** হলো একটি ইন্টারেক্টিভ স্মার্ট ডেস্কটপ এআই সঙ্গী (AI Desktop Companion)। এটি সরাসরি ESP32-C6 মাইক্রোকন্ট্রোলার থেকে Google Gemini 2.0 Flash API-তে ভয়েস অডিও পাঠিয়ে কথা বোঝে, বাংলায় এবং ইংরেজিতে মিষ্টি কণ্ঠে উত্তর দেয়, এবং সাথে সাথে 1.3" 240×240 IPS ডিসপ্লেতে কিউট কার্টুন মাসকট (Kitsune/Fox Mascot — **Kiko**) এর অ্যানিমেশন ও রিয়েল-টাইম লিপ-সিঙ্ক (Lip-sync) প্রদর্শন করে।

---

## 🛠️ Hardware Requirements (প্রয়োজনীয় হার্ডওয়্যার)

| Component | Model / Spec | Connection Type |
|---|---|---|
| **Microcontroller** | ESP32-C6-DevKitC-1 (WiFi 6 + BLE 5.3 + RISC-V) | — |
| **Display** | 1.3" IPS LCD 240×240 RGB (ST7789 Driver) | SPI (DMA Supported) |
| **Microphone** | INMP441 Omnidirectional Digital Mic | I2S0 (Audio Input) |
| **Speaker DAC** | MAX98357A Class-D Mono I2S Amplifier (3W) | I2S1 (Audio Output) |
| **Speaker** | 4Ω 3W or 8Ω 2W Mini Speaker (30-40mm) | Wired to MAX98357A |
| **Push-to-Talk Button** | Momentary Push Button (Tactile Switch) | GPIO 9 (Pull-up) |
| **Power Supply** | 5V 2A USB-C Cable / Power Adapter | USB-C Port |

---

## 🔌 Complete Wiring Diagram (সার্কিট ও পিন কানেকশন)

### 1. ST7789 240×240 IPS LCD Display (SPI)
```
┌────────────────────┬──────────────────────┐
│ ST7789 Pin         │ ESP32-C6 Pin         │
├────────────────────┼──────────────────────┤
│ VCC                │ 3.3V                 │
│ GND                │ GND                  │
│ SCL / SCLK         │ GPIO 6               │
│ SDA / MOSI         │ GPIO 7               │
│ RES / RST          │ GPIO 20              │
│ DC / RS            │ GPIO 19              │
│ CS / SS            │ GPIO 18              │
│ BLK / BL           │ GPIO 21 (PWM BL)     │
└────────────────────┴──────────────────────┘
```

### 2. INMP441 I2S Digital Microphone (Audio In)
```
┌────────────────────┬──────────────────────┐
│ INMP441 Pin        │ ESP32-C6 Pin         │
├────────────────────┼──────────────────────┤
│ VDD                │ 3.3V                 │
│ GND                │ GND                  │
│ L/R (Channel)      │ GND (Left Channel)   │
│ SCK / BCLK         │ GPIO 2               │
│ WS / LRCLK         │ GPIO 3               │
│ SD / DATA OUT      │ GPIO 4               │
└────────────────────┴──────────────────────┘
```

### 3. MAX98357A I2S Class-D DAC Amplifier (Audio Out)
```
┌────────────────────┬──────────────────────┐
│ MAX98357A Pin      │ ESP32-C6 Pin         │
├────────────────────┼──────────────────────┤
│ VIN                │ 5V (or 3.3V)         │
│ GND                │ GND                  │
│ GAIN               │ GND (Default 9dB)    │
│ SD_MODE            │ Leave unconnected    │
│ BCLK               │ GPIO 22              │
│ LRC / LRCLK        │ GPIO 23              │
│ DIN / DATA IN      │ GPIO 15              │
└────────────────────┴──────────────────────┘
* স্পিকারের (+) ও (-) তার MAX98357A এর (+ / -) স্ক্রু টার্মিনালে কানেক্ট করুন।
```

### 4. Push-To-Talk Button & Misc
```
┌────────────────────┬──────────────────────┐
│ Button / Switch    │ ESP32-C6 Pin         │
├────────────────────┼──────────────────────┤
│ Action Button      │ GPIO 9 to GND        │
│ Status RGB LED     │ GPIO 8 (On-board)    │
└────────────────────┴──────────────────────┘
```

---

## 🚀 Getting Started (সেটআপ ও কনফিগারেশন)

### ধাপ ১: কনফিগারেশন ফাইল আপডেট
[`include/config.h`](file:///d:/Bondhu%20AI/include/config.h) ফাইলে আপনার Wi-Fi এবং API কী দিন:

```cpp
#define WIFI_SSID           "Your_Home_WiFi"
#define WIFI_PASSWORD       "Your_WiFi_Password"

#define GEMINI_API_KEY      "AIzaSy..." // Google AI Studio Key
#define GOOGLE_TTS_API_KEY  "AIzaSy..." // Google Cloud TTS Key
#define OPENWEATHER_API_KEY "YOUR_KEY"  // Optional
```

### ধাপ ২: PlatformIO দিয়ে Build এবং Flash
VS Code বা Antigravity IDE-তে প্রজেক্ট ওপেন করে PlatformIO টার্মিনালে রান করুন:

```bash
# Firmware Compile
pio run

# Firmware Upload to ESP32-C6
pio run --target upload

# Serial Monitor দেখা
pio run --target monitor
```

---

## 🗣️ Voice Commands & Interactions (ভয়েস ইন্টারেকশন)

- **সাধারণ আলাপ (Bilingual Chat)**:
  - *"হ্যালো বন্ধু! কেমন আছো?"* → বন্ধু কিউট গলায় উত্তর দেবে এবং খুশি অভিব্যক্তি দেখাবে।
  - *"Hello Bondhu! What are you doing?"* → English cheerfully replies!
- **ডিসপ্লে কমান্ড (Display Control)**:
  - *"আমাকে সময় দেখাও"* → ডিসপ্লেতে Clock Overlay ভেসে উঠবে।
  - *"আজকের আবহাওয়া কেমন?"* → Weather card ও তাপমাত্রা দেখাবে।
  - *"কালকের কাজের তালিকা দেখাও"* → To-Do List মোড ওপেন হবে।
  - *"থিম পরিবর্তন করো / নাইট মোড দাও"* → ডিসপ্লের কালার প্যালেট Cyberpunk বা Pastel মোডে শিফট হবে।
- **স্মার্ট হোম কন্ট্রোল (Smart Home Actions)**:
  - *"ঘরের লাইট অন করো"* → Light ON সিগন্যাল ও কার্ড ভিউ।
  - *"ফ্যান বন্ধ করো"* → Fan OFF সিগন্যাল।
  - *"২ মিনিটের টাইমার দাও"* → ২ মিনিটের কাউন্টডাউন শুরু হবে এবং সময় শেষে অ্যালার্ম রিং বাজবে।

---

## 🎨 Mascot Expressions (কার্টুন অ্যানিমেশন মোডসমূহ)

| Emotion | Screen Animation | Audio Feedback |
|---|---|---|
| **IDLE** | স্বাভাবিক শ্বাস-প্রশ্বাস (Gentle breathing sine wave) ও চোখ পিটপিট (Blinking) | — |
| **LISTENING** | কান খাড়া করা (Twitching ears), বড় উজ্জ্বল চোখ এবং সাউন্ড পালস | Wake Chime (C-E-G) |
| **THINKING** | চোখ ওপরের দিকে, চারপাশে ঘূর্ণায়মান সাইবার অরবিট পার্টিকেলস | Processing Beep |
| **SPEAKING** | মাইক্রোফোনের অডিও লেভেলের সাথে ডায়নামিক লিপ-সিঙ্ক (Lip-sync :3) | High Quality TTS Voice |
| **HAPPY** | অ্যানিমে আনন্দের চোখ (⌒ ⌒), ওপরে ভাসমান লাভ হার্ট (Floating Hearts) | Cheerful Tone |
| **SAD** | ঝুলন্ত কান, কান্নার মতো ড্রুপ চোখ ও পানির ফোঁটা | Gentle Tone |
| **SLEEPING** | বন্ধ বাঁকানো চোখ এবং ভাসমান "Zzz" বাবলস | Peaceful Mode |

---

## 📂 Project Architecture

```
d:/Bondhu AI/
├── platformio.ini              # PlatformIO configuration (ESP32-C6, LovyanGFX, ArduinoJson)
├── partitions_custom.csv       # Custom OTA & Flash partition table
├── include/
│   ├── config.h                # Pinout, WiFi, API keys, sample rates
│   └── types.h                 # Emotion enums, display structs, AI packets
├── src/
│   ├── main.cpp                # System bootstrap, FreeRTOS 30FPS task scheduler
│   ├── audio/
│   │   ├── mic_driver.cpp/.h   # INMP441 I2S DMA input driver with silence VAD
│   │   └── speaker_driver.cpp/.h # MAX98357A I2S driver + live lip-sync amplitude extractor
│   ├── display/
│   │   ├── st7789_driver.cpp/.h # LovyanGFX 240x240 DMA canvas driver
│   │   ├── animation_engine.cpp/.h # 30FPS mascot state machine & procedural vector renderer
│   │   └── ui_manager.cpp/.h   # Clock, weather, to-do, speech bubbles, status bar overlays
│   ├── ai/
│   │   ├── gemini_client.cpp/.h # Gemini 2.0 Flash multimodal audio API + JSON parser
│   │   ├── tts_client.cpp/.h    # Google Cloud TTS client (Bangla 'bn-IN' & English 'en-IN')
│   │   └── context_manager.cpp/.h # Multi-turn chat memory and NVS persistent settings
│   ├── smart_home/
│   │   └── home_controller.cpp/.h # Light, fan, alarm, and countdown timer dispatcher
│   └── system/
│       ├── wifi_manager.cpp/.h  # Auto-reconnecting WiFi manager
│       ├── ntp_client.cpp/.h    # BST (GMT+6) time sync client
│       └── weather_client.cpp/.h # OpenWeatherMap temperature client
└── tools/
    ├── sprite_converter.py     # PNG to RGB565 C-header array converter
    └── simulate_api.py         # PC testing script for Gemini API key & prompt validation
```

---

## 🛠️ Python Development Tools

```bash
# ১. জেমিনি প্রম্পট এবং API কী পিসি থেকে টেস্ট করতে:
python tools/simulate_api.py

# ২. কাস্টম PNG ইমেজকে 240x240 RGB565 হেডারে কনভার্ট করতে:
python tools/sprite_converter.py my_mascot.png my_mascot.h mascot_frames
```

---
**Enjoy building Project Bondhu! 🐾**
