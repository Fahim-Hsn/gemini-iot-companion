#include "st7789_driver.h"

DisplayDriver displayDriver;

DisplayDriver::DisplayDriver() : _isInitialized(false) {}

DisplayDriver::~DisplayDriver() {}

bool DisplayDriver::begin() {
    if (_isInitialized) return true;

    // Explicitly power on Backlight (BLK) pin
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);

    if (PIN_LCD_RST >= 0) {
        pinMode(PIN_LCD_RST, OUTPUT);
        digitalWrite(PIN_LCD_RST, LOW);
        delay(20);
        digitalWrite(PIN_LCD_RST, HIGH);
        delay(50);
    }

    _gfx.init();
    _gfx.setRotation(0);
    _gfx.setColorDepth(16); // RGB565

    setBrightness(255); // Max brightness
    clear(0x10A2);      // Draw nice dark mascot background immediately
    
    _isInitialized = true;
    log_i("ST7789 7-pin 240x240 Display initialized directly with zero RAM footprint.");
    return true;
}

void DisplayDriver::setBrightness(uint8_t brightness) {
    _gfx.setBrightness(brightness);
}

void DisplayDriver::clear(uint16_t color) {
    _gfx.fillScreen(color);
}
