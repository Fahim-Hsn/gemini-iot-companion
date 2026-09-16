#pragma once

#include <Arduino.h>
#include "types.h"
#include "st7789_driver.h"

class AnimationEngine {
public:
    AnimationEngine();
    ~AnimationEngine();

    void begin();
    
    // Set current emotional state
    void setEmotion(MascotEmotion emotion, uint32_t holdDurationMs = 0);
    MascotEmotion getEmotion() const { return _currentEmotion; }

    // Set UI Theme
    void setTheme(UITheme theme);
    UITheme getTheme() const { return _theme; }

    // Main animation update tick (call at ~30 FPS)
    void update(float mouthLipSyncLevel = 0.0f);

    // Render mascot frame onto canvas
    void render(LGFX_Sprite& canvas, float mouthLevel);

private:
    MascotEmotion _currentEmotion;
    MascotEmotion _defaultEmotion;
    UITheme       _theme;
    uint32_t      _stateStartTime;
    uint32_t      _holdDurationMs;

    // Animation internal counters & easing
    uint32_t      _frameCount;
    float         _breathOffset;
    float         _earTwitchOffset;
    float         _tailWagAngle;
    bool          _isBlinking;
    uint32_t      _nextBlinkTime;
    uint32_t      _blinkStartTime;
    float         _thinkOrbAngle;

    // Color palette according to current theme
    uint16_t      _colBackground;
    uint16_t      _colFurPrimary;     // Warm Fox Orange / Cream
    uint16_t      _colFurSecondary;   // Chest White / Ear Inner
    uint16_t      _colEarInner;       // Soft Pink
    uint16_t      _colBlush;          // Rosy Pink
    uint16_t      _colEyeOuter;       // Deep Espresso / Night Navy
    uint16_t      _colEyeHighlight;   // Pure White Sparkle
    uint16_t      _colNoseMouth;      // Dark Cocoa

    void updatePalette();
    void drawFoxEars(LGFX_Sprite& canvas, int cx, int cy, float earAngle);
    void drawFoxHead(LGFX_Sprite& canvas, int cx, int cy);
    void drawFoxCheeksAndBlush(LGFX_Sprite& canvas, int cx, int cy);
    void drawFoxEyes(LGFX_Sprite& canvas, int cx, int cy, MascotEmotion emotion, bool isBlinking);
    void drawFoxMouth(LGFX_Sprite& canvas, int cx, int cy, MascotEmotion emotion, float mouthLevel);
    void drawEmotionParticles(LGFX_Sprite& canvas, int cx, int cy, MascotEmotion emotion);
};

extern AnimationEngine animationEngine;
