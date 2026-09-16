#include "st7789_driver.h"

DisplayDriver displayDriver;

DisplayDriver::DisplayDriver() : _canvas(&_gfx), _isInitialized(false) {}

DisplayDriver::~DisplayDriver() {
    _canvas.deleteSprite();
}

bool DisplayDriver::begin() {
    if (_isInitialized) return true;

    _gfx.init();
    _gfx.setRotation(0);
    _gfx.setColorDepth(16); // RGB565

    // Initialize 240x240 double buffer sprite canvas (PSRAM / Internal RAM)
    _canvas.setColorDepth(16);
    void* buffer = _canvas.createSprite(LCD_WIDTH, LCD_HEIGHT);
    if (!buffer) {
        log_e("Failed to allocate 240x240 frame buffer for Canvas!");
        return false;
    }

    setBrightness(220); // 85% default brightness
    clear(0x0000);
    _isInitialized = true;
    log_i("ST7789 240x240 Display & Canvas initialized successfully.");
    return true;
}

void DisplayDriver::setBrightness(uint8_t brightness) {
    _gfx.setBrightness(brightness);
}

void DisplayDriver::clear(uint16_t color) {
    _canvas.fillScreen(color);
}

void DisplayDriver::pushCanvas() {
    if (!_isInitialized) return;
    _canvas.pushSprite(0, 0);
}
