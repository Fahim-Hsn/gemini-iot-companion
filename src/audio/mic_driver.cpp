#include "mic_driver.h"
#include <cmath>

MicDriver micDriver;

MicDriver::MicDriver() : _isInitialized(false), _i2sPort(I2S_NUM_0) {}

MicDriver::~MicDriver() {
    end();
}

bool MicDriver::begin() {
    if (_isInitialized) {
        i2s_driver_uninstall(_i2sPort);
        _isInitialized = false;
    }

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = MIC_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // INMP441 default mono L/R tied
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = PIN_MIC_SCK,
        .ws_io_num = PIN_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = PIN_MIC_SD
    };

    esp_err_t err = i2s_driver_install(_i2sPort, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        log_e("Failed to install I2S driver for Mic: %d", err);
        return false;
    }

    err = i2s_set_pin(_i2sPort, &pin_config);
    if (err != ESP_OK) {
        log_e("Failed to set I2S pins for Mic: %d", err);
        i2s_driver_uninstall(_i2sPort);
        return false;
    }

    i2s_zero_dma_buffer(_i2sPort);
    _isInitialized = true;
    log_i("INMP441 Mic initialized on I2S0 (SCK:%d, WS:%d, SD:%d) @ %dHz", 
          PIN_MIC_SCK, PIN_MIC_WS, PIN_MIC_SD, MIC_SAMPLE_RATE);
    return true;
}

void MicDriver::end() {
    if (_isInitialized) {
        i2s_zero_dma_buffer(_i2sPort);
        i2s_driver_uninstall(_i2sPort);
        _isInitialized = false;

        pinMode(PIN_MIC_SCK, OUTPUT);
        digitalWrite(PIN_MIC_SCK, LOW);
        pinMode(PIN_MIC_WS, OUTPUT);
        digitalWrite(PIN_MIC_WS, LOW);
    }
}

size_t MicDriver::readRaw(int16_t* samples, size_t sampleCount) {
    if (!_isInitialized || !samples) return 0;

    size_t bytesRead = 0;
    esp_err_t err = i2s_read(_i2sPort, (void*)samples, sampleCount * sizeof(int16_t), &bytesRead, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        return bytesRead / sizeof(int16_t);
    }
    return 0;
}

float MicDriver::calculateEnergy(const int16_t* samples, size_t count) {
    if (!samples || count == 0) return 0.0f;
    double sumSquares = 0.0;
    for (size_t i = 0; i < count; i++) {
        // Boost digital gain slightly for INMP441 sensitivity
        int32_t val = (int32_t)samples[i] << 2;
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        sumSquares += (double)(val * val);
    }
    return (float)sqrt(sumSquares / (double)count);
}

bool MicDriver::isVoiceActive(float currentEnergy, float threshold) {
    return (currentEnergy >= threshold);
}

bool MicDriver::startRecording(uint8_t* outBuffer, size_t maxBytes, size_t* outWrittenBytes, uint32_t maxDurationMs) {
    if (!_isInitialized || !outBuffer || !outWrittenBytes) return false;

    *outWrittenBytes = 0;
    const size_t CHUNK_SAMPLES = 256;
    int16_t chunkBuffer[CHUNK_SAMPLES];

    // Clear stale DMA buffer
    i2s_zero_dma_buffer(_i2sPort);

    uint32_t startTime = millis();
    uint32_t silenceStart = 0;
    bool speechDetectedOnce = false;
    const float VOICE_THRESHOLD = 300.0f;
    const uint32_t SILENCE_TIMEOUT_MS = 1400; // 1.4 seconds of silence triggers end of sentence

    log_i("Listening for voice speech...");

    while ((millis() - startTime) < maxDurationMs) {
        size_t samplesRead = readRaw(chunkBuffer, CHUNK_SAMPLES);
        if (samplesRead == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        // Apply gain amplification to samples
        for (size_t i = 0; i < samplesRead; i++) {
            int32_t amplified = ((int32_t)chunkBuffer[i]) * 4;
            if (amplified > 32767) amplified = 32767;
            if (amplified < -32768) amplified = -32768;
            chunkBuffer[i] = (int16_t)amplified;
        }

        // Check voice energy
        float energy = calculateEnergy(chunkBuffer, samplesRead);
        bool hasVoice = isVoiceActive(energy, VOICE_THRESHOLD);

        if (hasVoice) {
            speechDetectedOnce = true;
            silenceStart = 0;
        } else {
            if (speechDetectedOnce) {
                if (silenceStart == 0) {
                    silenceStart = millis();
                } else if ((millis() - silenceStart) > SILENCE_TIMEOUT_MS) {
                    log_i("Silence detected after speech. Finished recording.");
                    break;
                }
            }
        }

        // Append to output buffer
        size_t bytesToCopy = samplesRead * sizeof(int16_t);
        if ((*outWrittenBytes + bytesToCopy) <= maxBytes) {
            memcpy(outBuffer + *outWrittenBytes, chunkBuffer, bytesToCopy);
            *outWrittenBytes += bytesToCopy;
        } else {
            log_w("Audio buffer full (%u bytes)", (unsigned)*outWrittenBytes);
            break;
        }
    }

    log_i("Recorded total %u bytes of audio.", (unsigned)*outWrittenBytes);
    return (*outWrittenBytes > (MIC_SAMPLE_RATE * sizeof(int16_t) / 2)); // Return true if at least 0.5s captured
}
