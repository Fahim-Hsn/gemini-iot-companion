#include "animation_engine.h"
#include <cmath>

AnimationEngine animationEngine;

AnimationEngine::AnimationEngine() 
    : _currentEmotion(MascotEmotion::IDLE),
      _defaultEmotion(MascotEmotion::IDLE),
      _theme(UITheme::MIDNIGHT_FOX),
      _stateStartTime(0),
      _holdDurationMs(0),
      _frameCount(0),
      _breathOffset(0.0f),
      _earTwitchOffset(0.0f),
      _tailWagAngle(0.0f),
      _isBlinking(false),
      _nextBlinkTime(3000),
      _blinkStartTime(0),
      _thinkOrbAngle(0.0f) {
    updatePalette();
}

AnimationEngine::~AnimationEngine() {}

void AnimationEngine::begin() {
    _stateStartTime = millis();
    _nextBlinkTime = millis() + random(2500, 5500);
    updatePalette();
}

void AnimationEngine::setEmotion(MascotEmotion emotion, uint32_t holdDurationMs) {
    _currentEmotion = emotion;
    _stateStartTime = millis();
    _holdDurationMs = holdDurationMs;
}

void AnimationEngine::setTheme(UITheme theme) {
    _theme = theme;
    updatePalette();
}

void AnimationEngine::updatePalette() {
    switch (_theme) {
        case UITheme::CYBERPUNK_DARK:
            _colBackground   = 0x0843; // Deep dark navy
            _colFurPrimary   = 0xFD20; // Neon Orange
            _colFurSecondary = 0xFFFF; // Crisp White
            _colEarInner     = 0xF81F; // Neon Magenta
            _colBlush        = 0xF814;
            _colEyeOuter     = 0x07FF; // Cyan Cyber eyes
            _colEyeHighlight = 0xFFFF;
            _colNoseMouth    = 0x0000;
            break;

        case UITheme::NEKO_PASTEL:
            _colBackground   = 0xF7BE; // Pastel Lavender Pink
            _colFurPrimary   = 0xFEE8; // Cream Peach
            _colFurSecondary = 0xFFFF;
            _colEarInner     = 0xFC78; // Soft Rose
            _colBlush        = 0xFB56;
            _colEyeOuter     = 0x4208; // Soft Mocha
            _colEyeHighlight = 0xFFFF;
            _colNoseMouth    = 0x4A49;
            break;

        case UITheme::NATURE_GREEN:
            _colBackground   = 0x1A25; // Dark Forest Green
            _colFurPrimary   = 0xFC80; // Golden Amber
            _colFurSecondary = 0xFFF9;
            _colEarInner     = 0xFD70;
            _colBlush        = 0xFBA0;
            _colEyeOuter     = 0x1903;
            _colEyeHighlight = 0xFFFF;
            _colNoseMouth    = 0x10A2;
            break;

        case UITheme::MIDNIGHT_FOX:
        default:
            _colBackground   = 0x10A2; // Midnight Charcoal Navy
            _colFurPrimary   = 0xFCA0; // Vibrant Fox Orange
            _colFurSecondary = 0xFFFF; // Snow White
            _colEarInner     = 0xFBAF; // Blossom Pink
            _colBlush        = 0xFA48; // Peach Blush
            _colEyeOuter     = 0x18C3; // Deep Espresso
            _colEyeHighlight = 0xFFFF;
            _colNoseMouth    = 0x2104;
            break;
    }
}

void AnimationEngine::update(float mouthLipSyncLevel) {
    _frameCount++;
    uint32_t now = millis();

    // Check hold duration timeout to revert to IDLE
    if (_holdDurationMs > 0 && (now - _stateStartTime) > _holdDurationMs) {
        _currentEmotion = _defaultEmotion;
        _holdDurationMs = 0;
    }

    // Breathing sine oscillation (2.5 second gentle period)
    float timeSec = (float)now / 1000.0f;
    _breathOffset = sinf(timeSec * 2.5f) * 2.0f;

    // Tail/Ear twitching physics
    if (_currentEmotion == MascotEmotion::LISTENING) {
        _earTwitchOffset = sinf(timeSec * 12.0f) * 3.5f;
    } else {
        _earTwitchOffset = sinf(timeSec * 1.8f) * 1.0f;
    }

    // Blink timer logic
    if (!_isBlinking && now >= _nextBlinkTime && _currentEmotion != MascotEmotion::SLEEPING) {
        _isBlinking = true;
        _blinkStartTime = now;
    } else if (_isBlinking && (now - _blinkStartTime) > 160) {
        _isBlinking = false;
        _nextBlinkTime = now + random(2200, 6000);
    }

    // Thinking particle orbit
    _thinkOrbAngle += 0.08f;
    if (_thinkOrbAngle > 2.0f * M_PI) _thinkOrbAngle -= 2.0f * M_PI;
}

void AnimationEngine::render(LGFX_Sprite& sprite, float mouthLevel) {
    int centerX = SPRITE_W / 2; // 90
    int centerY = (SPRITE_H / 2) + (int)_breathOffset + 14; // ~94

    // Clear offscreen sprite with themed background
    sprite.fillScreen(_colBackground);

    // 1. Draw Fox Ears (Behind head)
    drawFoxEars(sprite, centerX, centerY, _earTwitchOffset);

    // 2. Draw Fox Head Shape
    drawFoxHead(sprite, centerX, centerY);

    // 3. Draw Cheeks and Blush
    drawFoxCheeksAndBlush(sprite, centerX, centerY);

    // 4. Draw Expressive Eyes
    drawFoxEyes(sprite, centerX, centerY, _currentEmotion, _isBlinking);

    // 5. Draw Nose and Mouth with Lip Sync
    drawFoxMouth(sprite, centerX, centerY, _currentEmotion, mouthLevel);

    // 6. Draw Special Emotional Particles
    drawEmotionParticles(sprite, centerX, centerY, _currentEmotion);
}

void AnimationEngine::drawFoxEars(LGFX_Sprite& sprite, int cx, int cy, float earAngle) {
    // Left Ear
    int le_x1 = cx - 58, le_y1 = cy - 22;
    int le_x2 = cx - 25, le_y2 = cy - 50;
    int le_x3 = cx - 72 + (int)earAngle, le_y3 = cy - 82;

    sprite.fillTriangle(le_x1, le_y1, le_x2, le_y2, le_x3, le_y3, _colFurPrimary);
    sprite.fillTriangle(le_x1 + 8, le_y1 - 4, le_x2 - 3, le_y2 - 2, le_x3 + 7, le_y3 + 12, _colEarInner);

    // Right Ear
    int re_x1 = cx + 58, re_y1 = cy - 22;
    int re_x2 = cx + 25, re_y2 = cy - 50;
    int re_x3 = cx + 72 - (int)earAngle, re_y3 = cy - 82;

    sprite.fillTriangle(re_x1, re_y1, re_x2, re_y2, re_x3, re_y3, _colFurPrimary);
    sprite.fillTriangle(re_x1 - 8, re_y1 - 4, re_x2 + 3, re_y2 - 2, re_x3 - 7, re_y3 + 12, _colEarInner);
}

void AnimationEngine::drawFoxHead(LGFX_Sprite& sprite, int cx, int cy) {
    // Main round head base
    sprite.fillEllipse(cx, cy - 6, 60, 50, _colFurPrimary);

    // Fluffy side cheek tufts
    sprite.fillTriangle(cx - 56, cy - 8, cx - 75, cy + 6, cx - 38, cy + 24, _colFurPrimary);
    sprite.fillTriangle(cx + 56, cy - 8, cx + 75, cy + 6, cx + 38, cy + 24, _colFurPrimary);

    // White Muzzle / Chest Fur mask
    sprite.fillEllipse(cx, cy + 10, 42, 32, _colFurSecondary);
    sprite.fillTriangle(cx - 30, cy + 6, cx, cy - 6, cx + 30, cy + 6, _colFurSecondary);
}

void AnimationEngine::drawFoxCheeksAndBlush(LGFX_Sprite& sprite, int cx, int cy) {
    // Rosy Pink Cheeks
    sprite.fillCircle(cx - 40, cy + 14, 9, _colBlush);
    sprite.fillCircle(cx + 40, cy + 14, 9, _colBlush);

    // Cute subtle whisker dots
    sprite.fillCircle(cx - 24, cy + 13, 2, _colFurPrimary);
    sprite.fillCircle(cx - 30, cy + 16, 2, _colFurPrimary);
    sprite.fillCircle(cx + 24, cy + 13, 2, _colFurPrimary);
    sprite.fillCircle(cx + 30, cy + 16, 2, _colFurPrimary);
}

void AnimationEngine::drawFoxEyes(LGFX_Sprite& sprite, int cx, int cy, MascotEmotion emotion, bool isBlinking) {
    int eyeL_X = cx - 24;
    int eyeR_X = cx + 24;
    int eyeY   = cy - 5;

    if (isBlinking || emotion == MascotEmotion::SLEEPING) {
        sprite.drawArc(eyeL_X, eyeY + 4, 10, 8, 200, 340, _colEyeOuter);
        sprite.drawArc(eyeR_X, eyeY + 4, 10, 8, 200, 340, _colEyeOuter);
        return;
    }

    switch (emotion) {
        case MascotEmotion::HAPPY:
        case MascotEmotion::EXCITED:
            sprite.fillArc(eyeL_X, eyeY + 5, 12, 7, 200, 340, _colEyeOuter);
            sprite.fillArc(eyeR_X, eyeY + 5, 12, 7, 200, 340, _colEyeOuter);
            break;

        case MascotEmotion::SAD:
            sprite.fillEllipse(eyeL_X, eyeY + 2, 10, 12, _colEyeOuter);
            sprite.fillEllipse(eyeR_X, eyeY + 2, 10, 12, _colEyeOuter);
            sprite.fillCircle(eyeL_X - 8, eyeY + 12, 3, 0x5DFF);
            break;

        case MascotEmotion::CONFUSED:
            sprite.fillEllipse(eyeL_X, eyeY, 11, 14, _colEyeOuter);
            sprite.fillCircle(eyeL_X + 2, eyeY - 3, 3, _colEyeHighlight);
            sprite.drawArc(eyeR_X, eyeY + 3, 10, 8, 210, 330, _colEyeOuter);
            break;

        case MascotEmotion::THINKING:
            sprite.fillEllipse(eyeL_X, eyeY - 3, 10, 12, _colEyeOuter);
            sprite.fillEllipse(eyeR_X, eyeY - 3, 10, 12, _colEyeOuter);
            sprite.fillCircle(eyeL_X - 2, eyeY - 6, 3, _colEyeHighlight);
            sprite.fillCircle(eyeR_X - 2, eyeY - 6, 3, _colEyeHighlight);
            break;

        case MascotEmotion::LISTENING:
            sprite.fillEllipse(eyeL_X, eyeY - 2, 12, 15, _colEyeOuter);
            sprite.fillEllipse(eyeR_X, eyeY - 2, 12, 15, _colEyeOuter);
            sprite.fillCircle(eyeL_X - 3, eyeY - 6, 4, _colEyeHighlight);
            sprite.fillCircle(eyeL_X + 3, eyeY + 3, 2, _colEyeHighlight);
            sprite.fillCircle(eyeR_X - 3, eyeY - 6, 4, _colEyeHighlight);
            sprite.fillCircle(eyeR_X + 3, eyeY + 3, 2, _colEyeHighlight);
            break;

        case MascotEmotion::IDLE:
        case MascotEmotion::SPEAKING:
        default:
            sprite.fillEllipse(eyeL_X, eyeY, 10, 13, _colEyeOuter);
            sprite.fillEllipse(eyeR_X, eyeY, 10, 13, _colEyeOuter);
            sprite.fillCircle(eyeL_X - 2, eyeY - 4, 3, _colEyeHighlight);
            sprite.fillCircle(eyeL_X + 2, eyeY + 2, 2, _colEyeHighlight);
            sprite.fillCircle(eyeR_X - 2, eyeY - 4, 3, _colEyeHighlight);
            sprite.fillCircle(eyeR_X + 2, eyeY + 2, 2, _colEyeHighlight);
            break;
    }
}

void AnimationEngine::drawFoxMouth(LGFX_Sprite& sprite, int cx, int cy, MascotEmotion emotion, float mouthLevel) {
    int noseX = cx;
    int noseY = cy + 4;

    sprite.fillTriangle(noseX - 4, noseY - 2, noseX + 4, noseY - 2, noseX, noseY + 3, _colNoseMouth);
    int mouthY = noseY + 5;

    if (mouthLevel > 0.08f || emotion == MascotEmotion::SPEAKING) {
        int openH = (int)(mouthLevel * 16.0f) + 4;
        if (openH > 20) openH = 20;
        int openW = 9 + (openH / 2);

        sprite.fillEllipse(cx, mouthY + (openH / 2), openW, openH, 0x8800);
        sprite.fillEllipse(cx, mouthY + openH - 2, openW - 3, openH / 2, 0xFB56);
        sprite.drawEllipse(cx, mouthY + (openH / 2), openW, openH, _colNoseMouth);
    } else {
        switch (emotion) {
            case MascotEmotion::HAPPY:
            case MascotEmotion::EXCITED:
                sprite.drawArc(cx - 5, mouthY + 1, 5, 4, 20, 160, _colNoseMouth);
                sprite.drawArc(cx + 5, mouthY + 1, 5, 4, 20, 160, _colNoseMouth);
                break;

            case MascotEmotion::SAD:
                sprite.drawArc(cx, mouthY + 7, 9, 7, 200, 340, _colNoseMouth);
                break;

            case MascotEmotion::CONFUSED:
                sprite.drawLine(cx - 7, mouthY + 3, cx, mouthY + 5, _colNoseMouth);
                sprite.drawLine(cx, mouthY + 5, cx + 7, mouthY + 2, _colNoseMouth);
                break;

            default:
                sprite.drawArc(cx - 4, mouthY, 4, 3, 30, 160, _colNoseMouth);
                sprite.drawArc(cx + 4, mouthY, 4, 3, 20, 150, _colNoseMouth);
                break;
        }
    }
}

void AnimationEngine::drawEmotionParticles(LGFX_Sprite& sprite, int cx, int cy, MascotEmotion emotion) {
    uint32_t now = millis();

    if (emotion == MascotEmotion::HAPPY || emotion == MascotEmotion::EXCITED) {
        float floatY = sinf((float)now / 300.0f) * 4.0f;
        int hx = cx + 52, hy = cy - 40 + (int)floatY;
        sprite.fillCircle(hx - 3, hy, 4, 0xF814);
        sprite.fillCircle(hx + 3, hy, 4, 0xF814);
        sprite.fillTriangle(hx - 6, hy + 2, hx + 6, hy + 2, hx, hy + 8, 0xF814);
    } else if (emotion == MascotEmotion::THINKING) {
        for (int i = 0; i < 3; i++) {
            float angle = _thinkOrbAngle + (i * (2.0f * M_PI / 3.0f));
            int ox = cx + (int)(cosf(angle) * 62.0f);
            int oy = cy - 22 + (int)(sinf(angle) * 18.0f);
            sprite.fillCircle(ox, oy, 3, 0x07FF);
            sprite.fillCircle(ox, oy, 1, 0xFFFF);
        }
    } else if (emotion == MascotEmotion::SLEEPING) {
        int zOffset = (_frameCount * 2) % 50;
        int zx = cx + 38 + (zOffset / 3);
        int zy = cy - 30 - zOffset;
        sprite.setTextColor(0xAD7F);
        sprite.setTextSize(1);
        sprite.drawString("z", zx - 8, zy + 12);
        sprite.drawString("Z", zx, zy);
    } else if (emotion == MascotEmotion::LISTENING) {
        int pulseRadius = 20 + ((_frameCount * 3) % 18);
        sprite.drawCircle(cx, cy - 25, pulseRadius, 0xFD20);
    }
}
