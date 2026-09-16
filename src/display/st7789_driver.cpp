#include "st7789_driver.h"

DisplayDriver displayDriver;

DisplayDriver::DisplayDriver() : _mascotSprite(&_gfx), _isInitialized(false) {}

DisplayDriver::~DisplayDriver() {
    _mascotSprite.deleteSprite();
}

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
    clear(0x10A2);      // Draw nice dark mascot background once

    // Create a compact 180x160 offscreen sprite (only 57KB RAM) for flicker-free rendering
    _mascotSprite.setColorDepth(16);
    void* ptr = _mascotSprite.createSprite(SPRITE_W, SPRITE_H);
    if (!ptr) {
        log_e("Failed to create mascot sprite, falling back to direct render");
    } else {
        log_i("Mascot sprite allocated (%dx%d, 57KB) for butter-smooth animation.", SPRITE_W, SPRITE_H);
    }
    
    _isInitialized = true;
    return true;
}

void DisplayDriver::setBrightness(uint8_t brightness) {
    _gfx.setBrightness(brightness);
}

void DisplayDriver::clear(uint16_t color) {
    _gfx.fillScreen(color);
}

void DisplayDriver::pushMascotSprite(int x, int y) {
    _mascotSprite.pushSprite(x, y);
}
