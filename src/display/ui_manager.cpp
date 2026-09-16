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

void UIManager::renderOverlay(LGFX_Sprite& canvas) {
    uint32_t now = millis();

    // Check display mode timeout
    if (_modeDurationMs > 0 && (now - _modeStartTime) > _modeDurationMs) {
        _currentMode = DisplayMode::MASCOT_ONLY;
        _modeDurationMs = 0;
    }

    // Check speech bubble timeout
    if (_hasSpeechBubble && _speechDurationMs > 0 && (now - _speechStartTime) > _speechDurationMs) {
        _hasSpeechBubble = false;
    }

    // 1. Draw persistent top status bar
    drawStatusBar(canvas);

    // 2. Draw active mode overlay
    switch (_currentMode) {
        case DisplayMode::CLOCK_OVERLAY:
            drawClockOverlay(canvas);
            break;

        case DisplayMode::WEATHER_OVERLAY:
            drawWeatherOverlay(canvas);
            break;

        case DisplayMode::TODO_LIST:
            drawTodoListOverlay(canvas);
            break;

        case DisplayMode::CUSTOM_TEXT:
            drawCardOverlay(canvas);
            break;

        case DisplayMode::SPEECH_BUBBLE:
        case DisplayMode::MASCOT_ONLY:
        default:
            if (_hasSpeechBubble) {
                drawSpeechBubbleOverlay(canvas);
            }
            break;
    }
}

void UIManager::drawStatusBar(LGFX_Sprite& canvas) {
    // Semi-transparent / dark header bar
    canvas.fillRect(0, 0, LCD_WIDTH, 20, 0x0821);

    // Device Name / Status
    canvas.setTextColor(0xDEDB, 0x0821);
    canvas.setTextSize(1);
    canvas.drawString(_statusText, 8, 5);

    // WiFi Icon
    if (_wifiConnected) {
        canvas.drawCircle(LCD_WIDTH - 20, 10, 3, 0x07E0); // Green dot
        canvas.drawArc(LCD_WIDTH - 20, 10, 6, 5, 220, 320, 0x07E0);
    } else {
        canvas.drawCircle(LCD_WIDTH - 20, 10, 3, 0xF800); // Red dot
        canvas.drawLine(LCD_WIDTH - 23, 7, LCD_WIDTH - 17, 13, 0xF800);
    }
}

void UIManager::drawClockOverlay(LGFX_Sprite& canvas) {
    // Glassmorphism card at bottom
    int cardY = LCD_HEIGHT - 65;
    canvas.fillRoundRect(12, cardY, LCD_WIDTH - 24, 55, 10, 0x0842);
    canvas.drawRoundRect(12, cardY, LCD_WIDTH - 24, 55, 10, 0x39E7);

    // Time text
    canvas.setTextColor(0xFFFF, 0x0842);
    canvas.setTextSize(2);
    canvas.drawString(_currentTime, 24, cardY + 10);

    // Date text
    canvas.setTextColor(0x9CF3, 0x0842);
    canvas.setTextSize(1);
    canvas.drawString(_currentDate, 24, cardY + 34);
}

void UIManager::drawWeatherOverlay(LGFX_Sprite& canvas) {
    int cardY = LCD_HEIGHT - 68;
    canvas.fillRoundRect(12, cardY, LCD_WIDTH - 24, 58, 10, 0x10A4);
    canvas.drawRoundRect(12, cardY, LCD_WIDTH - 24, 58, 10, 0x5AEB);

    // Weather Icon Placeholder (Sun/Cloud)
    canvas.fillCircle(38, cardY + 28, 12, 0xFD20); // Amber Sun

    // Temp & Condition
    canvas.setTextColor(0xFFFF, 0x10A4);
    canvas.setTextSize(2);
    canvas.drawString(String(_currentTempC) + "°C", 62, cardY + 10);

    canvas.setTextColor(0xBDF7, 0x10A4);
    canvas.setTextSize(1);
    canvas.drawString(_weatherCondition + " • " + _weatherCity, 62, cardY + 34);
}

void UIManager::drawTodoListOverlay(LGFX_Sprite& canvas) {
    // Full screen translucent overlay
    canvas.fillRoundRect(10, 26, LCD_WIDTH - 20, LCD_HEIGHT - 36, 8, 0x0821);
    canvas.drawRoundRect(10, 26, LCD_WIDTH - 20, LCD_HEIGHT - 36, 8, 0x4208);

    canvas.setTextColor(0xFD20, 0x0821);
    canvas.setTextSize(1);
    canvas.drawString("TODAY'S REMINDERS", 22, 34);

    canvas.setTextColor(0xFFFF, 0x0821);
    canvas.drawString("[x] 10:00 AM - Check emails", 20, 56);
    canvas.drawString("[ ] 02:30 PM - Project meeting", 20, 78);
    canvas.drawString("[ ] 06:00 PM - Evening walk", 20, 100);
    canvas.drawString("[ ] 08:00 PM - Learn AI coding", 20, 122);
}

void UIManager::drawSpeechBubbleOverlay(LGFX_Sprite& canvas) {
    if (_speechText.length() == 0) return;

    // Elegant bottom dialogue bubble
    int boxX = 10;
    int boxY = LCD_HEIGHT - 75;
    int boxW = LCD_WIDTH - 20;
    int boxH = 65;

    canvas.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x0842);
    canvas.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x52AA);

    // Character Tag
    canvas.setTextColor(0xFCA0, 0x0842);
    canvas.setTextSize(1);
    canvas.drawString("Bondhu:", boxX + 8, boxY + 6);

    // Dialogue text wrapping
    canvas.setTextColor(0xFFFF, 0x0842);
    
    // Simple line wrap for preview
    String line1 = _speechText;
    String line2 = "";
    if (line1.length() > 28) {
        int splitIdx = line1.lastIndexOf(' ', 28);
        if (splitIdx > 0) {
            line2 = line1.substring(splitIdx + 1);
            line1 = line1.substring(0, splitIdx);
        }
    }

    canvas.drawString(line1, boxX + 8, boxY + 22);
    if (line2.length() > 0) {
        canvas.drawString(line2, boxX + 8, boxY + 38);
    }
}

void UIManager::drawCardOverlay(LGFX_Sprite& canvas) {
    int boxX = 14;
    int boxY = LCD_HEIGHT - 65;
    int boxW = LCD_WIDTH - 28;
    int boxH = 55;

    canvas.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x18C3);
    canvas.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x632C);

    canvas.setTextColor(0xFFFF, 0x18C3);
    canvas.setTextSize(1);
    canvas.drawString(_cardTitle, boxX + 10, boxY + 10);

    canvas.setTextColor(0x9CF3, 0x18C3);
    canvas.drawString(_cardSubtitle, boxX + 10, boxY + 30);
}
