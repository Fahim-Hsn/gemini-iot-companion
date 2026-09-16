#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"

// LovyanGFX Display class customized for ESP32-C6 & ST7789
class LGFX_ST7789_C6 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789      _panel_instance;
    lgfx::Bus_SPI           _bus_instance;
    lgfx::Light_PWM         _light_instance;

public:
    LGFX_ST7789_C6() {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = LCD_SPI_FREQ;
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = PIN_LCD_SCLK;
            cfg.pin_mosi = PIN_LCD_MOSI;
            cfg.pin_miso = -1;
            cfg.pin_dc   = PIN_LCD_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = PIN_LCD_CS;
            cfg.pin_rst          = PIN_LCD_RST;
            cfg.pin_busy         = -1;
            cfg.panel_width      = LCD_WIDTH;
            cfg.panel_height     = LCD_HEIGHT;
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = false;
            cfg.invert           = true;  // ST7789 IPS typically needs inversion
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;
            _panel_instance.config(cfg);
        }

        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = PIN_LCD_BL;
            cfg.invert = false;
            cfg.freq   = 44100;
            cfg.pwm_channel = 7;
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
    LGFX_Sprite& getCanvas() { return _canvas; }
    
    void pushCanvas();
    void clear(uint16_t color = 0x0000);

private:
    LGFX_ST7789_C6 _gfx;
    LGFX_Sprite    _canvas;
    bool           _isInitialized;
};

extern DisplayDriver displayDriver;
