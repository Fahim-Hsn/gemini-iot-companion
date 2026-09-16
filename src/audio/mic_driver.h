#pragma once

#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"

class MicDriver {
public:
    MicDriver();
    ~MicDriver();

    bool begin();
    void end();

    // Start recording audio into internal buffer until silence is detected or max duration reached
    bool startRecording(uint8_t* outBuffer, size_t maxBytes, size_t* outWrittenBytes, uint32_t maxDurationMs = (MIC_RECORD_MAX_SEC * 1000));
    
    // Read raw audio chunk
    size_t readRaw(int16_t* samples, size_t sampleCount);

    // Calculate RMS energy of audio frame (for Voice Activity Detection / VAD)
    float calculateEnergy(const int16_t* samples, size_t count);

    // Check if voice is present
    bool isVoiceActive(float currentEnergy, float threshold = 350.0f);

    bool isInitialized() const { return _isInitialized; }

private:
    bool _isInitialized;
    i2s_port_t _i2sPort;
};

extern MicDriver micDriver;
