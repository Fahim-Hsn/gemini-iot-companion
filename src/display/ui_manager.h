#pragma once

#include <Arduino.h>
#include "types.h"
#include "st7789_driver.h"

class UIManager {
public:
    UIManager();
    ~UIManager();

    void begin();
    
    void setDisplayMode(DisplayMode mode, uint32_t durationMs = 0);
    DisplayMode getDisplayMode() const { return _currentMode; }

    void setSpeechBubble(const String& text, uint32_t durationMs = 6000);
    void clearSpeechBubble();

    void setCardInfo(const String& title, const String& subtitle, uint32_t durationMs = 5000);

    // Render active overlay directly on top of display
    void renderOverlay(LGFX_ST7789_C6& gfx);

    void setTime(const String& timeStr, const String& dateStr);
    void setWeather(int tempC, const String& condition, const String& city);
    void setWiFiStatus(bool isConnected, int rssi = 0);
    void setSystemStatusText(const String& status);

private:
    DisplayMode  _currentMode;
    uint32_t     _modeStartTime;
    uint32_t     _modeDurationMs;

    String       _speechText;
    uint32_t     _speechStartTime;
    uint32_t     _speechDurationMs;
    bool         _hasSpeechBubble;

    String       _cardTitle;
    String       _cardSubtitle;
    uint32_t     _cardStartTime;
    uint32_t     _cardDurationMs;

    String       _currentTime;
    String       _currentDate;
    int          _currentTempC;
    String       _weatherCondition;
    String       _weatherCity;
    bool         _wifiConnected;
    int          _wifiRssi;
    String       _statusText;

    void drawStatusBar(LGFX_ST7789_C6& gfx);
    void drawClockOverlay(LGFX_ST7789_C6& gfx);
    void drawWeatherOverlay(LGFX_ST7789_C6& gfx);
    void drawTodoListOverlay(LGFX_ST7789_C6& gfx);
    void drawSpeechBubbleOverlay(LGFX_ST7789_C6& gfx);
    void drawCardOverlay(LGFX_ST7789_C6& gfx);
};

extern UIManager uiManager;
