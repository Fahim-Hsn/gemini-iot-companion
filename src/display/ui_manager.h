#pragma once

#include <Arduino.h>
#include "types.h"
#include "st7789_driver.h"
#include "animation_engine.h"

class UIManager {
public:
    UIManager();
    ~UIManager();

    void begin();
    
    // Set current active display overlay mode
    void setDisplayMode(DisplayMode mode, uint32_t durationMs = 0);
    DisplayMode getDisplayMode() const { return _currentMode; }

    // Set custom speech bubble text to display
    void setSpeechBubble(const String& text, uint32_t durationMs = 6000);
    void clearSpeechBubble();

    // Set custom title/subtitle card
    void setCardInfo(const String& title, const String& subtitle, uint32_t durationMs = 5000);

    // Render active overlay on top of mascot canvas
    void renderOverlay(LGFX_Sprite& canvas);

    // Update time & weather cache
    void setTime(const String& timeStr, const String& dateStr);
    void setWeather(int tempC, const String& condition, const String& city);
    void setWiFiStatus(bool isConnected, int rssi = 0);
    void setSystemStatusText(const String& status);

private:
    DisplayMode  _currentMode;
    uint32_t     _modeStartTime;
    uint32_t     _modeDurationMs;

    // Speech bubble state
    String       _speechText;
    uint32_t     _speechStartTime;
    uint32_t     _speechDurationMs;
    bool         _hasSpeechBubble;

    // Card info
    String       _cardTitle;
    String       _cardSubtitle;
    uint32_t     _cardStartTime;
    uint32_t     _cardDurationMs;

    // Cached system data
    String       _currentTime;
    String       _currentDate;
    int          _currentTempC;
    String       _weatherCondition;
    String       _weatherCity;
    bool         _wifiConnected;
    int          _wifiRssi;
    String       _statusText;

    void drawStatusBar(LGFX_Sprite& canvas);
    void drawClockOverlay(LGFX_Sprite& canvas);
    void drawWeatherOverlay(LGFX_Sprite& canvas);
    void drawTodoListOverlay(LGFX_Sprite& canvas);
    void drawSpeechBubbleOverlay(LGFX_Sprite& canvas);
    void drawCardOverlay(LGFX_Sprite& canvas);
};

extern UIManager uiManager;
