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
            _colBackground   = 0x10A2; // Midnight Charcoal Navy (0x10A2)
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
    _breathOffset = sinf(timeSec * 2.5f) * 2.8f;

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

void AnimationEngine::render(LGFX_Sprite& canvas, float mouthLevel) {
    // Fill themed background
    canvas.fillScreen(_colBackground);

    int centerX = LCD_WIDTH / 2;
    int centerY = (LCD_HEIGHT / 2) + (int)_breathOffset + 8;

    // 1. Draw Fox Ears (Behind head)
    drawFoxEars(canvas, centerX, centerY, _earTwitchOffset);

    // 2. Draw Fox Head Shape
    drawFoxHead(canvas, centerX, centerY);

    // 3. Draw Cheeks and Blush
    drawFoxCheeksAndBlush(canvas, centerX, centerY);

    // 4. Draw Expressive Eyes
    drawFoxEyes(canvas, centerX, centerY, _currentEmotion, _isBlinking);

    // 5. Draw Nose and Mouth with Lip Sync
    drawFoxMouth(canvas, centerX, centerY, _currentEmotion, mouthLevel);

    // 6. Draw Special Emotional Particles (Hearts, Zzz, Orbiting Dots, Sparkles)
    drawEmotionParticles(canvas, centerX, centerY, _currentEmotion);
}

void AnimationEngine::drawFoxEars(LGFX_Sprite& canvas, int cx, int cy, float earAngle) {
    // Left Ear
    int le_x1 = cx - 68, le_y1 = cy - 25;
    int le_x2 = cx - 30, le_y2 = cy - 58;
    int le_x3 = cx - 85 + (int)earAngle, le_y3 = cy - 95;

    canvas.fillTriangle(le_x1, le_y1, le_x2, le_y2, le_x3, le_y3, _colFurPrimary);
    // Inner Pink
    canvas.fillTriangle(le_x1 + 10, le_y1 - 5, le_x2 - 4, le_y2 - 2, le_x3 + 8, le_y3 + 14, _colEarInner);

    // Right Ear
    int re_x1 = cx + 68, re_y1 = cy - 25;
    int re_x2 = cx + 30, re_y2 = cy - 58;
    int re_x3 = cx + 85 - (int)earAngle, re_y3 = cy - 95;

    canvas.fillTriangle(re_x1, re_y1, re_x2, re_y2, re_x3, re_y3, _colFurPrimary);
    // Inner Pink
    canvas.fillTriangle(re_x1 - 10, re_y1 - 5, re_x2 + 4, re_y2 - 2, re_x3 - 8, re_y3 + 14, _colEarInner);
}

void AnimationEngine::drawFoxHead(LGFX_Sprite& canvas, int cx, int cy) {
    // Main round head base
    canvas.fillEllipse(cx, cy - 8, 70, 58, _colFurPrimary);

    // Fluffy side cheek tufts
    canvas.fillTriangle(cx - 65, cy - 10, cx - 88, cy + 8, cx - 45, cy + 28, _colFurPrimary);
    canvas.fillTriangle(cx + 65, cy - 10, cx + 88, cy + 8, cx + 45, cy + 28, _colFurPrimary);

    // White Muzzle / Chest Fur mask
    canvas.fillEllipse(cx, cy + 12, 48, 36, _colFurSecondary);
    canvas.fillTriangle(cx - 36, cy + 8, cx, cy - 8, cx + 36, cy + 8, _colFurSecondary);
}

void AnimationEngine::drawFoxCheeksAndBlush(LGFX_Sprite& canvas, int cx, int cy) {
    // Rosy Pink Cheeks
    canvas.fillCircle(cx - 46, cy + 16, 11, _colBlush);
    canvas.fillCircle(cx + 46, cy + 16, 11, _colBlush);

    // Cute subtle whisker dots
    canvas.fillCircle(cx - 28, cy + 15, 2, _colFurPrimary);
    canvas.fillCircle(cx - 35, cy + 19, 2, _colFurPrimary);
    canvas.fillCircle(cx + 28, cy + 15, 2, _colFurPrimary);
    canvas.fillCircle(cx + 35, cy + 19, 2, _colFurPrimary);
}

void AnimationEngine::drawFoxEyes(LGFX_Sprite& canvas, int cx, int cy, MascotEmotion emotion, bool isBlinking) {
    int eyeL_X = cx - 28;
    int eyeR_X = cx + 28;
    int eyeY   = cy - 6;

    if (isBlinking || emotion == MascotEmotion::SLEEPING) {
        // Closed curved happy/sleeping line eyes ( ^  ^ )
        canvas.drawArc(eyeL_X, eyeY + 4, 12, 10, 200, 340, _colEyeOuter);
        canvas.drawArc(eyeR_X, eyeY + 4, 12, 10, 200, 340, _colEyeOuter);
        return;
    }

    switch (emotion) {
        case MascotEmotion::HAPPY:
        case MascotEmotion::EXCITED:
            // Curved anime joyful eyes (⌒ ⌒)
            canvas.fillArc(eyeL_X, eyeY + 6, 14, 8, 200, 340, _colEyeOuter);
            canvas.fillArc(eyeR_X, eyeY + 6, 14, 8, 200, 340, _colEyeOuter);
            break;

        case MascotEmotion::SAD:
            // Droopy sad eyes
            canvas.fillEllipse(eyeL_X, eyeY + 2, 11, 14, _colEyeOuter);
            canvas.fillEllipse(eyeR_X, eyeY + 2, 11, 14, _colEyeOuter);
            // Tear sparkle on left eye
            canvas.fillCircle(eyeL_X - 10, eyeY + 14, 4, 0x5DFF);
            break;

        case MascotEmotion::CONFUSED:
            // One big eye, one squinted eye (?_o)
            canvas.fillEllipse(eyeL_X, eyeY, 13, 16, _colEyeOuter);
            canvas.fillCircle(eyeL_X + 2, eyeY - 4, 4, _colEyeHighlight);
            canvas.drawArc(eyeR_X, eyeY + 4, 11, 9, 210, 330, _colEyeOuter);
            break;

        case MascotEmotion::THINKING:
            // Eyes looking slightly upward
            canvas.fillEllipse(eyeL_X, eyeY - 4, 12, 14, _colEyeOuter);
            canvas.fillEllipse(eyeR_X, eyeY - 4, 12, 14, _colEyeOuter);
            canvas.fillCircle(eyeL_X - 2, eyeY - 8, 4, _colEyeHighlight);
            canvas.fillCircle(eyeR_X - 2, eyeY - 8, 4, _colEyeHighlight);
            break;

        case MascotEmotion::LISTENING:
            // Wide sparkling attentive anime pupils
            canvas.fillEllipse(eyeL_X, eyeY - 2, 14, 17, _colEyeOuter);
            canvas.fillEllipse(eyeR_X, eyeY - 2, 14, 17, _colEyeOuter);
            // Big double sparkle highlight
            canvas.fillCircle(eyeL_X - 3, eyeY - 7, 5, _colEyeHighlight);
            canvas.fillCircle(eyeL_X + 4, eyeY + 4, 2, _colEyeHighlight);
            canvas.fillCircle(eyeR_X - 3, eyeY - 7, 5, _colEyeHighlight);
            canvas.fillCircle(eyeR_X + 4, eyeY + 4, 2, _colEyeHighlight);
            break;

        case MascotEmotion::IDLE:
        case MascotEmotion::SPEAKING:
        default:
            // Standard cute cartoon eyes
            canvas.fillEllipse(eyeL_X, eyeY, 12, 15, _colEyeOuter);
            canvas.fillEllipse(eyeR_X, eyeY, 12, 15, _colEyeOuter);
            // Eye Highlights
            canvas.fillCircle(eyeL_X - 3, eyeY - 5, 4, _colEyeHighlight);
            canvas.fillCircle(eyeL_X + 3, eyeY + 3, 2, _colEyeHighlight);
            canvas.fillCircle(eyeR_X - 3, eyeY - 5, 4, _colEyeHighlight);
            canvas.fillCircle(eyeR_X + 3, eyeY + 3, 2, _colEyeHighlight);
            break;
    }
}

void AnimationEngine::drawFoxMouth(LGFX_Sprite& canvas, int cx, int cy, MascotEmotion emotion, float mouthLevel) {
    int noseX = cx;
    int noseY = cy + 4;

    // Small cute triangular cocoa nose
    canvas.fillTriangle(noseX - 4, noseY - 2, noseX + 4, noseY - 2, noseX, noseY + 3, _colNoseMouth);

    int mouthY = noseY + 6;

    if (mouthLevel > 0.08f || emotion == MascotEmotion::SPEAKING) {
        // Animated open mouth with real-time lip sync level
        int openH = (int)(mouthLevel * 18.0f) + 4;
        if (openH > 22) openH = 22;
        int openW = 10 + (openH / 2);

        // Open mouth cavity (dark red/burgundy)
        canvas.fillEllipse(cx, mouthY + (openH / 2), openW, openH, 0x8800);
        // Pink tongue inside
        canvas.fillEllipse(cx, mouthY + openH - 2, openW - 3, openH / 2, 0xFB56);
        // Mouth outline
        canvas.drawEllipse(cx, mouthY + (openH / 2), openW, openH, _colNoseMouth);
    } else {
        // Closed / smiling cat/fox mouth (:3)
        switch (emotion) {
            case MascotEmotion::HAPPY:
            case MascotEmotion::EXCITED:
                // Big smile
                canvas.drawArc(cx - 6, mouthY + 1, 6, 5, 20, 160, _colNoseMouth);
                canvas.drawArc(cx + 6, mouthY + 1, 6, 5, 20, 160, _colNoseMouth);
                break;

            case MascotEmotion::SAD:
                // Downward frown
                canvas.drawArc(cx, mouthY + 8, 10, 8, 200, 340, _colNoseMouth);
                break;

            case MascotEmotion::CONFUSED:
                // Wavy mouth
                canvas.drawLine(cx - 8, mouthY + 3, cx, mouthY + 6, _colNoseMouth);
                canvas.drawLine(cx, mouthY + 6, cx + 8, mouthY + 2, _colNoseMouth);
                break;

            default:
                // Classic cute w-mouth (:3)
                canvas.drawArc(cx - 5, mouthY, 5, 4, 30, 160, _colNoseMouth);
                canvas.drawArc(cx + 5, mouthY, 5, 4, 20, 150, _colNoseMouth);
                break;
        }
    }
}

void AnimationEngine::drawEmotionParticles(LGFX_Sprite& canvas, int cx, int cy, MascotEmotion emotion) {
    uint32_t now = millis();

    if (emotion == MascotEmotion::HAPPY || emotion == MascotEmotion::EXCITED) {
        // Floating Heart on top right
        float floatY = sinf((float)now / 300.0f) * 4.0f;
        int hx = cx + 62, hy = cy - 45 + (int)floatY;
        canvas.fillCircle(hx - 4, hy, 5, 0xF814); // Pink-Red
        canvas.fillCircle(hx + 4, hy, 5, 0xF814);
        canvas.fillTriangle(hx - 8, hy + 2, hx + 8, hy + 2, hx, hy + 10, 0xF814);
    } else if (emotion == MascotEmotion::THINKING) {
        // Orbiting glowing dots / stars
        for (int i = 0; i < 3; i++) {
            float angle = _thinkOrbAngle + (i * (2.0f * M_PI / 3.0f));
            int ox = cx + (int)(cosf(angle) * 75.0f);
            int oy = cy - 25 + (int)(sinf(angle) * 22.0f);
            canvas.fillCircle(ox, oy, 4, 0x07FF); // Glowing cyan dots
            canvas.fillCircle(ox, oy, 2, 0xFFFF);
        }
    } else if (emotion == MascotEmotion::SLEEPING) {
        // Floating Zzz letters
        int zOffset = (_frameCount * 2) % 60;
        int zx = cx + 45 + (zOffset / 3);
        int zy = cy - 35 - zOffset;
        canvas.setTextColor(0xAD7F); // Light blue
        canvas.setTextSize(1);
        canvas.drawString("z", zx - 10, zy + 15);
        canvas.drawString("Z", zx, zy);
    } else if (emotion == MascotEmotion::LISTENING) {
        // Sound wave pulsing concentric arcs
        int pulseRadius = 25 + ((_frameCount * 3) % 20);
        canvas.drawCircle(cx, cy - 30, pulseRadius, 0xFD20); // Amber pulse
    }
}
