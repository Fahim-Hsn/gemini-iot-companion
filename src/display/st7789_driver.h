#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"

// LovyanGFX Display class customized for ESP32-C6 & 7-Pin ST7789
class LGFX_ST7789_C6 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789      _panel_instance;
    lgfx::Bus_SPI           _bus_instance;
    lgfx::Light_PWM         _light_instance;

public:
    LGFX_ST7789_C6() {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 3;           // Mode 3 is REQUIRED for CS-less ST7789 displays (CS tied to GND)
            cfg.freq_write = 20000000;  // 20MHz for clean noise-free breadboard SPI
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = PIN_LCD_SCLK; // GPIO 6
            cfg.pin_mosi = PIN_LCD_MOSI; // GPIO 7
            cfg.pin_miso = -1;
            cfg.pin_dc   = PIN_LCD_DC;   // GPIO 19
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = -1;           // 7-Pin module has CS tied to GND
            cfg.pin_rst          = PIN_LCD_RST;  // GPIO 1 (or 3V3)
            cfg.pin_busy         = -1;
            cfg.panel_width      = LCD_WIDTH;    // 240
            cfg.panel_height     = LCD_HEIGHT;   // 240
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = false;
            cfg.invert           = true;         // ST7789 IPS inversion
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;
            _panel_instance.config(cfg);
        }

        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = PIN_LCD_BL;             // GPIO 5
            cfg.invert = false;
            cfg.freq   = 44100;
            cfg.pwm_channel = 1;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

class DisplayDriver {
public:
    DisplayDriver();
    ~DisplayDriver();

    bool begin();
    void setBrightness(uint8_t brightness); // 0-255
    LGFX_ST7789_C6& getLGFX() { return _gfx; }
    
    void clear(uint16_t color = 0x0000);
    void startWrite() { _gfx.startWrite(); }
    void endWrite() { _gfx.endWrite(); }

private:
    LGFX_ST7789_C6 _gfx;
    bool           _isInitialized;
};

extern DisplayDriver displayDriver;
