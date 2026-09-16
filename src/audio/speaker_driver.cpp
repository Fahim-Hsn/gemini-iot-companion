#include "speaker_driver.h"
#include <cmath>

SpeakerDriver speakerDriver;

SpeakerDriver::SpeakerDriver() 
    : _isInitialized(false), 
      _i2sPort(I2S_NUM_1), 
      _volume(85), 
      _volumeScale(0.85f), 
      _isPlaying(false), 
      _currentMouthLevel(0.0f) {}

SpeakerDriver::~SpeakerDriver() {
    end();
}

bool SpeakerDriver::begin(uint32_t sampleRate) {
    if (_isInitialized) return true;

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // MAX98357A stereo frame
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = PIN_SPK_BCLK,
        .ws_io_num = PIN_SPK_LRC,
        .data_out_num = PIN_SPK_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(_i2sPort, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        log_e("Failed to install I2S driver for Speaker: %d", err);
        return false;
    }

    err = i2s_set_pin(_i2sPort, &pin_config);
    if (err != ESP_OK) {
        log_e("Failed to set I2S pins for Speaker: %d", err);
        i2s_driver_uninstall(_i2sPort);
        return false;
    }

    _isInitialized = true;
    log_i("MAX98357A Speaker initialized on I2S1 (BCLK:%d, LRC:%d, DIN:%d) @ %dHz",
          PIN_SPK_BCLK, PIN_SPK_LRC, PIN_SPK_DIN, sampleRate);
    return true;
}

void SpeakerDriver::end() {
    if (_isInitialized) {
        i2s_driver_uninstall(_i2sPort);
        _isInitialized = false;
    }
}

void SpeakerDriver::setVolume(uint8_t volumePercent) {
    if (volumePercent > 100) volumePercent = 100;
    _volume = volumePercent;
    _volumeScale = (float)_volume / 100.0f;
}

void SpeakerDriver::stopPlayback() {
    _isPlaying = false;
    _currentMouthLevel = 0.0f;
    i2s_zero_dma_buffer(_i2sPort);
}

size_t SpeakerDriver::writeChunk(const int16_t* samples, size_t sampleCount) {
    if (!_isInitialized || !samples) return 0;

    size_t bytesWritten = 0;
    esp_err_t err = i2s_write(_i2sPort, (const void*)samples, sampleCount * sizeof(int16_t), &bytesWritten, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        return bytesWritten / sizeof(int16_t);
    }
    return 0;
}

bool SpeakerDriver::playPCM(const uint8_t* pcmData, size_t dataSize, uint32_t sampleRate, bool isMono) {
    if (!_isInitialized || !pcmData || dataSize == 0) return false;

    // Adjust I2S clock if sample rate changes
    i2s_set_clk(_i2sPort, sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);

    _isPlaying = true;
    const size_t CHUNK_SIZE = 512;
    int16_t stereoBuffer[CHUNK_SIZE * 2];
    const int16_t* pcm16 = (const int16_t*)pcmData;
    size_t totalSamples = dataSize / sizeof(int16_t);

    for (size_t i = 0; i < totalSamples && _isPlaying; i += CHUNK_SIZE) {
        size_t samplesThisBlock = (i + CHUNK_SIZE <= totalSamples) ? CHUNK_SIZE : (totalSamples - i);
        double energyAcc = 0.0;

        for (size_t j = 0; j < samplesThisBlock; j++) {
            int16_t original = pcm16[i + j];
            int16_t scaled = (int16_t)(original * _volumeScale);

            stereoBuffer[j * 2] = scaled;     // Left
            stereoBuffer[j * 2 + 1] = scaled; // Right

            energyAcc += (double)(scaled * scaled);
        }

        // Calculate live amplitude for real-time lip sync animation
        float rms = (float)sqrt(energyAcc / (double)samplesThisBlock);
        _currentMouthLevel = (rms / 8000.0f);
        if (_currentMouthLevel > 1.0f) _currentMouthLevel = 1.0f;

        size_t bytesWritten = 0;
        i2s_write(_i2sPort, (const void*)stereoBuffer, samplesThisBlock * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    }

    _isPlaying = false;
    _currentMouthLevel = 0.0f;
    i2s_zero_dma_buffer(_i2sPort);
    return true;
}

void SpeakerDriver::playTone(float frequency, uint32_t durationMs, float volume) {
    if (!_isInitialized) return;

    uint32_t sampleRate = 24000;
    size_t totalSamples = (sampleRate * durationMs) / 1000;
    const size_t CHUNK = 256;
    int16_t buffer[CHUNK * 2];

    float phase = 0.0f;
    float phaseIncrement = (2.0f * M_PI * frequency) / (float)sampleRate;
    float effectiveVol = volume * _volumeScale * 16000.0f;

    for (size_t i = 0; i < totalSamples; i += CHUNK) {
        size_t n = (i + CHUNK <= totalSamples) ? CHUNK : (totalSamples - i);
        for (size_t j = 0; j < n; j++) {
            int16_t val = (int16_t)(sinf(phase) * effectiveVol);
            phase += phaseIncrement;
            if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;

            buffer[j * 2] = val;
            buffer[j * 2 + 1] = val;
        }
        size_t bytesWritten = 0;
        i2s_write(_i2sPort, (const void*)buffer, n * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    }
}

void SpeakerDriver::playWakeChime() {
    playTone(523.25f, 90, 0.4f); // C5
    playTone(659.25f, 90, 0.5f); // E5
    playTone(783.99f, 140, 0.6f); // G5
}

void SpeakerDriver::playReadyBeep() {
    playTone(880.0f, 60, 0.3f);  // A5
}

void SpeakerDriver::playSuccessChime() {
    playTone(587.33f, 80, 0.4f); // D5
    playTone(880.00f, 160, 0.5f); // A5
}

void SpeakerDriver::playErrorTone() {
    playTone(392.0f, 120, 0.5f); // G4
    playTone(329.63f, 200, 0.5f); // E4
}
