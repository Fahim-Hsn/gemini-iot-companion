#pragma once

#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"

class SpeakerDriver {
public:
    SpeakerDriver();
    ~SpeakerDriver();

    bool begin(uint32_t sampleRate = SPK_SAMPLE_RATE);
    void end();

    // Play PCM audio buffer (16-bit Mono/Stereo)
    bool playPCM(const uint8_t* pcmData, size_t dataSize, uint32_t sampleRate = 24000, bool isMono = true);

    // Write raw chunk to I2S buffer
    size_t writeChunk(const int16_t* samples, size_t sampleCount);

    // Play synthesized acoustic cues
    void playWakeChime();
    void playReadyBeep();
    void playSuccessChime();
    void playErrorTone();

    // Volume configuration (0 to 100)
    void setVolume(uint8_t volumePercent);
    uint8_t getVolume() const { return _volume; }

    // Lip sync helper: returns current audio playback amplitude (0.0 to 1.0)
    float getCurrentMouthLevel() const { return _currentMouthLevel; }

    bool isPlaying() const { return _isPlaying; }
    void stopPlayback();

private:
    bool _isInitialized;
    i2s_port_t _i2sPort;
    uint8_t _volume;
    float _volumeScale;
    volatile bool _isPlaying;
    volatile float _currentMouthLevel;

    void playTone(float frequency, uint32_t durationMs, float volume = 0.5f);
};

extern SpeakerDriver speakerDriver;
