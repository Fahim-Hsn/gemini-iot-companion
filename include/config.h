#pragma once

#include <Arduino.h>

// =========================================================================
// Project Bondhu - Hardware & System Configuration
// Target: ESP32-C6-DevKitC-1 + ST7789 240x240 + INMP441 + MAX98357A
// =========================================================================

// --- Import Private Secrets / .env if available ---
#if __has_include("secrets.h")
    #include "secrets.h"
#endif

// --- Device & Persona Information ---
#ifndef DEVICE_NAME
#define DEVICE_NAME             "Bondhu-AI"
#endif
#ifndef MASCOT_NAME
#define MASCOT_NAME             "Kiko"
#endif
#ifndef WAKE_WORD_DEFAULT
#define WAKE_WORD_DEFAULT       "Hey Bondhu"
#endif
#ifndef DEFAULT_USER_NAME
#define DEFAULT_USER_NAME       "Fahim"
#endif
#define TIMEZONE_OFFSET_SEC     (6 * 3600)  // GMT+6 for Bangladesh Standard Time
#define NTP_SERVER_1            "pool.ntp.org"
#define NTP_SERVER_2            "time.google.com"

// --- Wi-Fi Credentials ---
#ifndef WIFI_SSID
#define WIFI_SSID               "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
#endif
#define WIFI_CONNECT_TIMEOUT_MS 15000

// --- AI API Keys & Endpoints ---
#ifndef GEMINI_API_KEY
#define GEMINI_API_KEY          "YOUR_GEMINI_API_KEY"
#endif
#ifndef GEMINI_MODEL
#define GEMINI_MODEL            "gemini-flash-lite-latest"
#endif
#define GEMINI_API_HOST         "generativelanguage.googleapis.com"

#ifndef GOOGLE_TTS_API_KEY
#define GOOGLE_TTS_API_KEY      "YOUR_GOOGLE_CLOUD_API_KEY"
#endif
#define GOOGLE_TTS_HOST         "texttospeech.googleapis.com"
#define TTS_VOICE_BANGLA        "bn-IN-Standard-A"  // Cute Bengali voice
#define TTS_VOICE_ENGLISH       "en-IN-Standard-A"  // Cute English voice

#ifndef OPENWEATHER_API_KEY
#define OPENWEATHER_API_KEY     "YOUR_OPENWEATHER_KEY"
#endif
#ifndef WEATHER_CITY
#define WEATHER_CITY            "Dhaka"
#endif
#define WEATHER_COUNTRY_CODE    "BD"

// =========================================================================
// Pin Configuration (ESP32-C6-DevKitC-1 v1.2 Pinout)
// =========================================================================

// --- ST7789 1.3" 240x240 IPS Display (7-Pin Module: GND, VCC, SCL, SDA, RES, DC, BLK) ---
#define PIN_LCD_MOSI            7     // Right Pin 6 (SDA)
#define PIN_LCD_SCLK            6     // Right Pin 5 (SCL)
#define PIN_LCD_DC              19    // Left Pin 9 (DC)
#define PIN_LCD_RST             1     // Right Pin 8 (RES - or connect to 3V3)
#define PIN_LCD_BL              5     // Right Pin 4 (BLK - Backlight, or connect to 3V3)
#define LCD_WIDTH               240
#define LCD_HEIGHT              240
#define LCD_SPI_HOST            SPI2_HOST
#define LCD_SPI_FREQ            40000000 // 40MHz high speed SPI

// --- INMP441 Microphone (I2S0 In) ---
#define PIN_MIC_SCK             2     // Right Pin 12 (SCK / BCLK)
#define PIN_MIC_WS              3     // Right Pin 13 (WS / LRCLK)
#define PIN_MIC_SD              4     // Right Pin 3 (SD / Serial Data In)
#define MIC_SAMPLE_RATE         16000 // 16kHz for Voice / STT
#define MIC_BITS_PER_SAMPLE     16
#define MIC_RECORD_MAX_SEC      8     // Maximum recording window

// --- MAX98357A I2S DAC Amplifier (I2S1 Out) ---
#define PIN_SPK_BCLK            21    // Left Pin 7 (BCLK)
#define PIN_SPK_LRC             22    // Left Pin 6 (LRC / WS)
#define PIN_SPK_DIN             23    // Left Pin 5 (DIN / Data In)
#define SPK_SAMPLE_RATE         24000 // 24kHz / 16kHz for TTS playback

// --- Push Button & Interactions ---
#define PIN_BUTTON_ACTION       9     // Left Pin 11 (Boot button / Push-to-Talk)
#define PIN_STATUS_RGB          8     // Right Pin 9 (Onboard RGB LED)

// =========================================================================
// FreeRTOS Task Priorities & Stack Sizes
// =========================================================================
#define TASK_PRIO_AUDIO_IN      4     // High priority for audio capture
#define TASK_PRIO_AUDIO_OUT     4     // High priority for audio playback
#define TASK_PRIO_DISPLAY       2     // Medium priority for 30fps animation
#define TASK_PRIO_NETWORK       3     // High-medium for Gemini & TTS HTTPS
#define TASK_PRIO_SYSTEM        1     // Background routines, timers

#define STACK_SIZE_AUDIO_IN     (6 * 1024)
#define STACK_SIZE_AUDIO_OUT    (6 * 1024)
#define STACK_SIZE_DISPLAY      (6 * 1024)
#define STACK_SIZE_NETWORK      (16 * 1024)
#define STACK_SIZE_SYSTEM       (4 * 1024)
