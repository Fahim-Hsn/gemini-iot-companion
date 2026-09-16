#include "ui_manager.h"

UIManager uiManager;

UIManager::UIManager()
    : _currentMode(DisplayMode::MASCOT_ONLY),
      _modeStartTime(0),
      _modeDurationMs(0),
      _speechStartTime(0),
      _speechDurationMs(0),
      _hasSpeechBubble(false),
      _cardStartTime(0),
      _cardDurationMs(0),
      _currentTime("12:00 PM"),
      _currentDate("Wed, Sep 16"),
      _currentTempC(28),
      _weatherCondition("Sunny"),
      _weatherCity("Dhaka"),
      _wifiConnected(false),
      _wifiRssi(-60),
      _statusText("Ready") {}

UIManager::~UIManager() {}

void UIManager::begin() {}

void UIManager::setDisplayMode(DisplayMode mode, uint32_t durationMs) {
    _currentMode = mode;
    _modeStartTime = millis();
    _modeDurationMs = durationMs;
}

void UIManager::setSpeechBubble(const String& text, uint32_t durationMs) {
    _speechText = text;
    _speechStartTime = millis();
    _speechDurationMs = durationMs;
    _hasSpeechBubble = (text.length() > 0);
}

void UIManager::clearSpeechBubble() {
    _hasSpeechBubble = false;
    _speechText = "";
}

void UIManager::setCardInfo(const String& title, const String& subtitle, uint32_t durationMs) {
    _cardTitle = title;
    _cardSubtitle = subtitle;
    _cardStartTime = millis();
    _cardDurationMs = durationMs;
    setDisplayMode(DisplayMode::CUSTOM_TEXT, durationMs);
}

void UIManager::setTime(const String& timeStr, const String& dateStr) {
    _currentTime = timeStr;
    _currentDate = dateStr;
}

void UIManager::setWeather(int tempC, const String& condition, const String& city) {
    _currentTempC = tempC;
    _weatherCondition = condition;
    _weatherCity = city;
}

void UIManager::setWiFiStatus(bool isConnected, int rssi) {
    _wifiConnected = isConnected;
    _wifiRssi = rssi;
}

void UIManager::setSystemStatusText(const String& status) {
    _statusText = status;
}

void UIManager::renderOverlay(LGFX_ST7789_C6& gfx) {
    uint32_t now = millis();

    if (_modeDurationMs > 0 && (now - _modeStartTime) > _modeDurationMs) {
        _currentMode = DisplayMode::MASCOT_ONLY;
        _modeDurationMs = 0;
    }

    if (_hasSpeechBubble && _speechDurationMs > 0 && (now - _speechStartTime) > _speechDurationMs) {
        _hasSpeechBubble = false;
    }

    drawStatusBar(gfx);

    switch (_currentMode) {
        case DisplayMode::CLOCK_OVERLAY:
            drawClockOverlay(gfx);
            break;

        case DisplayMode::WEATHER_OVERLAY:
            drawWeatherOverlay(gfx);
            break;

        case DisplayMode::TODO_LIST:
            drawTodoListOverlay(gfx);
            break;

        case DisplayMode::CUSTOM_TEXT:
            drawCardOverlay(gfx);
            break;

        case DisplayMode::SPEECH_BUBBLE:
        case DisplayMode::MASCOT_ONLY:
        default:
            if (_hasSpeechBubble) {
                drawSpeechBubbleOverlay(gfx);
            }
            break;
    }
}

void UIManager::drawStatusBar(LGFX_ST7789_C6& gfx) {
    gfx.fillRect(0, 0, LCD_WIDTH, 20, 0x0821);

    gfx.setTextColor(0xDEDB, 0x0821);
    gfx.setTextSize(1);
    gfx.drawString(_statusText, 8, 5);

    if (_wifiConnected) {
        gfx.drawCircle(LCD_WIDTH - 20, 10, 3, 0x07E0);
        gfx.drawArc(LCD_WIDTH - 20, 10, 6, 5, 220, 320, 0x07E0);
    } else {
        gfx.drawCircle(LCD_WIDTH - 20, 10, 3, 0xF800);
        gfx.drawLine(LCD_WIDTH - 23, 7, LCD_WIDTH - 17, 13, 0xF800);
    }
}

void UIManager::drawClockOverlay(LGFX_ST7789_C6& gfx) {
    int cardY = LCD_HEIGHT - 65;
    gfx.fillRoundRect(12, cardY, LCD_WIDTH - 24, 55, 10, 0x0842);
    gfx.drawRoundRect(12, cardY, LCD_WIDTH - 24, 55, 10, 0x39E7);

    gfx.setTextColor(0xFFFF, 0x0842);
    gfx.setTextSize(2);
    gfx.drawString(_currentTime, 24, cardY + 10);

    gfx.setTextColor(0x9CF3, 0x0842);
    gfx.setTextSize(1);
    gfx.drawString(_currentDate, 24, cardY + 34);
}

void UIManager::drawWeatherOverlay(LGFX_ST7789_C6& gfx) {
    int cardY = LCD_HEIGHT - 68;
    gfx.fillRoundRect(12, cardY, LCD_WIDTH - 24, 58, 10, 0x10A4);
    gfx.drawRoundRect(12, cardY, LCD_WIDTH - 24, 58, 10, 0x5AEB);

    gfx.fillCircle(38, cardY + 28, 12, 0xFD20);

    gfx.setTextColor(0xFFFF, 0x10A4);
    gfx.setTextSize(2);
    gfx.drawString(String(_currentTempC) + "°C", 62, cardY + 10);

    gfx.setTextColor(0xBDF7, 0x10A4);
    gfx.setTextSize(1);
    gfx.drawString(_weatherCondition + " • " + _weatherCity, 62, cardY + 34);
}

void UIManager::drawTodoListOverlay(LGFX_ST7789_C6& gfx) {
    gfx.fillRoundRect(10, 26, LCD_WIDTH - 20, LCD_HEIGHT - 36, 8, 0x0821);
    gfx.drawRoundRect(10, 26, LCD_WIDTH - 20, LCD_HEIGHT - 36, 8, 0x4208);

    gfx.setTextColor(0xFD20, 0x0821);
    gfx.setTextSize(1);
    gfx.drawString("TODAY'S REMINDERS", 22, 34);

    gfx.setTextColor(0xFFFF, 0x0821);
    gfx.drawString("[x] 10:00 AM - Check emails", 20, 56);
    gfx.drawString("[ ] 02:30 PM - Project meeting", 20, 78);
    gfx.drawString("[ ] 06:00 PM - Evening walk", 20, 100);
    gfx.drawString("[ ] 08:00 PM - Learn AI coding", 20, 122);
}

void UIManager::drawSpeechBubbleOverlay(LGFX_ST7789_C6& gfx) {
    if (_speechText.length() == 0) return;

    int boxX = 10;
    int boxY = LCD_HEIGHT - 75;
    int boxW = LCD_WIDTH - 20;
    int boxH = 65;

    gfx.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x0842);
    gfx.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x52AA);

    gfx.setTextColor(0xFCA0, 0x0842);
    gfx.setTextSize(1);
    gfx.drawString("Bondhu:", boxX + 8, boxY + 6);

    gfx.setTextColor(0xFFFF, 0x0842);
    
    String line1 = _speechText;
    String line2 = "";
    if (line1.length() > 28) {
        int splitIdx = line1.lastIndexOf(' ', 28);
        if (splitIdx > 0) {
            line2 = line1.substring(splitIdx + 1);
            line1 = line1.substring(0, splitIdx);
        }
    }

    gfx.drawString(line1, boxX + 8, boxY + 22);
    if (line2.length() > 0) {
        gfx.drawString(line2, boxX + 8, boxY + 38);
    }
}

void UIManager::drawCardOverlay(LGFX_ST7789_C6& gfx) {
    int boxX = 14;
    int boxY = LCD_HEIGHT - 65;
    int boxW = LCD_WIDTH - 28;
    int boxH = 55;

    gfx.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x18C3);
    gfx.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x632C);

    gfx.setTextColor(0xFFFF, 0x18C3);
    gfx.setTextSize(1);
    gfx.drawString(_cardTitle, boxX + 10, boxY + 10);

    gfx.setTextColor(0x9CF3, 0x18C3);
    gfx.drawString(_cardSubtitle, boxX + 10, boxY + 30);
}
