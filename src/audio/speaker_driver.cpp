#include "speaker_driver.h"
#include <cmath>

SpeakerDriver speakerDriver;

SpeakerDriver::SpeakerDriver() 
    : _isInitialized(false), 
      _i2sPort(I2S_NUM_0), 
      _volume(85), 
      _volumeScale(0.85f), 
      _isPlaying(false), 
      _currentMouthLevel(0.0f),
      _currentSampleRate(SPK_SAMPLE_RATE) {}

SpeakerDriver::~SpeakerDriver() {
    end();
}

bool SpeakerDriver::begin(uint32_t sampleRate) {
    if (_isInitialized) {
        end();
    }

    _currentSampleRate = sampleRate;

    // Reset GPIO pads to clean hardware state before I2S assignment
    gpio_reset_pin((gpio_num_t)PIN_SPK_BCLK);
    gpio_reset_pin((gpio_num_t)PIN_SPK_LRC);
    gpio_reset_pin((gpio_num_t)PIN_SPK_DIN);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // MAX98357A stereo frame
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
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

    i2s_zero_dma_buffer(_i2sPort);
    _isInitialized = true;
    log_i("MAX98357A Speaker initialized on I2S0 (BCLK:%d, LRC:%d, DIN:%d) @ %uHz",
          PIN_SPK_BCLK, PIN_SPK_LRC, PIN_SPK_DIN, (unsigned)sampleRate);
    return true;
}

void SpeakerDriver::end() {
    if (_isInitialized) {
        _isPlaying = false;
        _currentMouthLevel = 0.0f;
        i2s_zero_dma_buffer(_i2sPort);
        i2s_driver_uninstall(_i2sPort);
        _isInitialized = false;

        // Drive pins to clean LOW output state so MAX98357A DAC does not float and click
        pinMode(PIN_SPK_BCLK, OUTPUT);
        digitalWrite(PIN_SPK_BCLK, LOW);
        pinMode(PIN_SPK_LRC, OUTPUT);
        digitalWrite(PIN_SPK_LRC, LOW);
        pinMode(PIN_SPK_DIN, OUTPUT);
        digitalWrite(PIN_SPK_DIN, LOW);
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
    if (_isInitialized) {
        i2s_zero_dma_buffer(_i2sPort);
    }
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

void SpeakerDriver::beginStreaming(uint32_t sampleRate) {
    if (!_isInitialized) {
        begin(sampleRate);
    }
    if (_currentSampleRate != sampleRate) {
        i2s_set_clk(_i2sPort, sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
        _currentSampleRate = sampleRate;
    }
    _isPlaying = true;
    _currentMouthLevel = 0.0f;
}

void SpeakerDriver::playPCMChunk(const int16_t* monoSamples, size_t sampleCount) {
    if (!_isInitialized || !monoSamples || sampleCount == 0 || !_isPlaying) return;

    const size_t MAX_BLOCK = 256;
    int16_t stereoBuffer[MAX_BLOCK * 2];

    for (size_t i = 0; i < sampleCount && _isPlaying; i += MAX_BLOCK) {
        size_t n = (i + MAX_BLOCK <= sampleCount) ? MAX_BLOCK : (sampleCount - i);
        double energyAcc = 0.0;

        for (size_t j = 0; j < n; j++) {
            int16_t original = monoSamples[i + j];
            int16_t scaled = (int16_t)(original * _volumeScale);

            stereoBuffer[j * 2]     = scaled; // Left
            stereoBuffer[j * 2 + 1] = scaled; // Right

            energyAcc += (double)(scaled * scaled);
        }

        // Live mouth amplitude tracking
        float rms = (float)sqrt(energyAcc / (double)n);
        _currentMouthLevel = (rms / 8000.0f);
        if (_currentMouthLevel > 1.0f) _currentMouthLevel = 1.0f;

        size_t bytesWritten = 0;
        i2s_write(_i2sPort, (const void*)stereoBuffer, n * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    }
}

void SpeakerDriver::endStreaming() {
    _isPlaying = false;
    _currentMouthLevel = 0.0f;
    if (_isInitialized) {
        i2s_zero_dma_buffer(_i2sPort);
    }
}

bool SpeakerDriver::playPCM(const uint8_t* pcmData, size_t dataSize, uint32_t sampleRate, bool isMono) {
    if (!_isInitialized || !pcmData || dataSize == 0) return false;

    // Detect and skip 44-byte WAV header if present
    size_t offset = 0;
    if (dataSize >= 44 && memcmp(pcmData, "RIFF", 4) == 0 && memcmp(pcmData + 8, "WAVE", 4) == 0) {
        offset = 44;
    }

    size_t effectiveSize = dataSize - offset;
    const int16_t* pcm16 = (const int16_t*)(pcmData + offset);
    size_t totalSamples = effectiveSize / sizeof(int16_t);

    beginStreaming(sampleRate);
    playPCMChunk(pcm16, totalSamples);
    endStreaming();
    return true;
}

void SpeakerDriver::playTone(float frequency, uint32_t durationMs, float volume) {
    if (!_isInitialized) return;

    if (_currentSampleRate != 24000) {
        i2s_set_clk(_i2sPort, 24000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
        _currentSampleRate = 24000;
    }

    bool outerPlaying = _isPlaying;
    _isPlaying = true;

    uint32_t sampleRate = 24000;
    size_t totalSamples = (sampleRate * durationMs) / 1000;
    const size_t CHUNK = 256;
    int16_t buffer[CHUNK * 2];

    float phase = 0.0f;
    float phaseIncrement = (2.0f * (float)M_PI * frequency) / (float)sampleRate;
    // Boost effective volume for loud output on MAX98357A
    float effectiveVol = volume * _volumeScale * 28000.0f;

    for (size_t i = 0; i < totalSamples && _isPlaying; i += CHUNK) {
        size_t n = (i + CHUNK <= totalSamples) ? CHUNK : (totalSamples - i);
        double energyAcc = 0.0;
        for (size_t j = 0; j < n; j++) {
            // Smooth attack and decay envelope to eliminate edge clicks
            float env = 1.0f;
            size_t sampleIdx = i + j;
            if (sampleIdx < 120) env = (float)sampleIdx / 120.0f;
            else if (sampleIdx + 120 > totalSamples) env = (float)(totalSamples - sampleIdx) / 120.0f;

            int16_t val = (int16_t)(sinf(phase) * effectiveVol * env);
            phase += phaseIncrement;
            if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;

            buffer[j * 2]     = val;
            buffer[j * 2 + 1] = val;
            energyAcc += (double)(val * val);
        }

        // Live mouth amplitude tracking
        float rms = (float)sqrt(energyAcc / (double)n);
        _currentMouthLevel = (rms / 15000.0f);
        if (_currentMouthLevel > 1.0f) _currentMouthLevel = 1.0f;

        size_t bytesWritten = 0;
        i2s_write(_i2sPort, (const void*)buffer, n * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    }

    if (!outerPlaying) {
        _isPlaying = false;
        _currentMouthLevel = 0.0f;
    }
}

void SpeakerDriver::playStartupSound() {
    log_i("Playing Bondhu Startup Chime...");
    playTone(440.0f, 80, 0.8f); // A4
    playTone(554.37f, 80, 0.8f); // C#5
    playTone(659.25f, 80, 0.9f); // E5
    playTone(880.0f, 160, 1.0f); // A5
}

void SpeakerDriver::playWakeChime() {
    playTone(523.25f, 80, 0.8f); // C5
    playTone(659.25f, 80, 0.9f); // E5
    playTone(783.99f, 130, 1.0f); // G5
}

void SpeakerDriver::playReadyBeep() {
    playTone(880.0f, 60, 0.8f);  // A5
}

void SpeakerDriver::playSuccessChime() {
    playTone(587.33f, 80, 0.8f); // D5
    playTone(880.00f, 150, 0.9f); // A5
}

void SpeakerDriver::playErrorTone() {
    playTone(392.0f, 100, 0.8f); // G4
    playTone(329.63f, 180, 0.8f); // E4
}

void SpeakerDriver::speakMascotVoice(const String& text) {
    if (!_isInitialized || text.length() == 0) return;

    log_i("Mascot speaking syllable voice for text (%u chars)...", (unsigned)text.length());
    _isPlaying = true;

    // Harmonic melody notes for cute speech cadence
    static const float voicePitches[] = {
        523.25f, // C5
        587.33f, // D5
        659.25f, // E5
        698.46f, // F5
        783.99f, // G5
        880.00f, // A5
        987.77f, // B5
        1046.50f // C6
    };

    size_t len = text.length();
    size_t i = 0;
    size_t spokenCount = 0;
    const size_t MAX_SPOKEN_SYLLABLES = 60; // Max syllables to keep speech lively and responsive

    while (i < len && _isPlaying && spokenCount < MAX_SPOKEN_SYLLABLES) {
        uint8_t b = (uint8_t)text[i];

        // Whitespace pause
        if (b == ' ' || b == '\t' || b == '\n' || b == '\r') {
            vTaskDelay(pdMS_TO_TICKS(40));
            i++;
            continue;
        }

        // Punctuation pause
        if (b == '.' || b == '!' || b == '?' || b == ',' || b == ';' || b == ':') {
            vTaskDelay(pdMS_TO_TICKS(110));
            i++;
            continue;
        }

        // Handle UTF-8 character unit (English 1 byte, Bengali 3 bytes)
        uint32_t charCode = b;
        size_t charLen = 1;
        if ((b & 0xE0) == 0xC0 && (i + 1 < len)) {
            charLen = 2;
            charCode = ((b & 0x1F) << 6) | (text[i + 1] & 0x3F);
        } else if ((b & 0xF0) == 0xE0 && (i + 2 < len)) {
            charLen = 3;
            charCode = ((b & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) | (text[i + 2] & 0x3F);
        } else if ((b & 0xF8) == 0xF0 && (i + 3 < len)) {
            charLen = 4;
            charCode = ((b & 0x07) << 18) | ((text[i + 1] & 0x3F) << 12) | ((text[i + 2] & 0x3F) << 6) | (text[i + 3] & 0x3F);
        }

        i += charLen;
        spokenCount++;

        // Determine pitch from charCode hash
        float freq = voicePitches[charCode % 8];
        if (b >= 'A' && b <= 'Z') freq *= 1.12f;

        // Play pleasant 60ms vocal syllable tone
        playTone(freq, 60, 0.95f);
        vTaskDelay(pdMS_TO_TICKS(8));
    }

    _isPlaying = false;
    _currentMouthLevel = 0.0f;
    i2s_zero_dma_buffer(_i2sPort);
}
