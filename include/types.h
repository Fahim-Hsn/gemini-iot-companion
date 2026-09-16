#pragma once

#include <Arduino.h>

// =========================================================================
// Project Bondhu - Core Types & Enums
// =========================================================================

// --- Mascot Animation States ---
enum class MascotEmotion {
    IDLE = 0,
    LISTENING,
    THINKING,
    SPEAKING,
    HAPPY,
    SAD,
    CONFUSED,
    SLEEPING,
    WAKING_UP,
    EXCITED
};

// --- Display Mode Overlays ---
enum class DisplayMode {
    MASCOT_ONLY = 0,
    CLOCK_OVERLAY,
    WEATHER_OVERLAY,
    TODO_LIST,
    SPEECH_BUBBLE,
    SMART_HOME_STATUS,
    MINI_GAME,
    CUSTOM_TEXT,
    PHOTO_FRAME
};

// --- Color Themes for UI ---
enum class UITheme {
    CYBERPUNK_DARK = 0,
    NEKO_PASTEL,
    NATURE_GREEN,
    WARM_SUNSET,
    DEEP_OCEAN,
    MIDNIGHT_FOX
};

// --- System State Machine ---
enum class SystemState {
    BOOTING = 0,
    CONNECTING_WIFI,
    STANDBY_IDLE,
    LISTENING_VOICE,
    CALLING_GEMINI,
    GENERATING_TTS,
    SPEAKING_RESPONSE,
    ERROR_STATE
};

// --- Smart Home Action Packet ---
struct SmartHomeCommand {
    String action;         // "light_on", "light_off", "fan_set", "set_alarm", "set_timer", "schedule_reminder"
    String target;         // "bedroom_light", "ceiling_fan", "main_alarm"
    int value;             // e.g. brightness %, timer in seconds
    String metadata;       // e.g. "Meeting with Team at 4pm"
    bool isValid;
};

// --- Display Command Packet ---
struct DisplayCommand {
    DisplayMode mode;
    MascotEmotion emotion;
    String title;
    String subtitle;
    String speechText;
    int durationMs;
    UITheme theme;
    bool isValid;
};

// --- Parsed Gemini Structured AI Response ---
struct AIResponse {
    String rawResponseText;
    String speechText;              // Text to speak out loud in TTS
    String languageCode;            // "bn" (Bangla) or "en" (English)
    MascotEmotion emotion;          // Target animation state
    DisplayCommand displayCmd;      // Changes to display screen
    SmartHomeCommand homeCmd;       // Smart home actions to execute
    bool isSuccess;
    String errorMessage;
};

// --- Personality Profile & Settings ---
struct PersonalityProfile {
    String userName;
    String mascotName;
    float playfulness;             // 0.0 (serious) to 1.0 (very cute/playful)
    bool proactiveGreeting;        // Morning/Evening proactive alerts
    int morningGreetingHour;       // e.g. 8 (8:00 AM)
    int eveningGreetingHour;       // e.g. 22 (10:00 PM)
    int speechVolume;              // 0 to 100
};
