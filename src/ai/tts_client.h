#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"

class TTSClient {
public:
    TTSClient();
    ~TTSClient();

    bool begin();
    
    // Synthesize speech from text and stream directly to speaker driver
    bool speakText(const String& text, const String& langCode = "bn");

    // Google Cloud Text-to-Speech
    bool speakGoogleCloudTTS(const String& text, const String& langCode = "bn");

    // VoiceRSS Free Human Voice (16kHz WAV streaming)
    bool speakVoiceRSS(const String& text, const String& langCode = "bn");

    // Synthesize text and return raw PCM buffer
    bool synthesize(const String& text, const String& langCode, uint8_t** outPCM, size_t* outPCMSize, uint32_t* outSampleRate);

private:
    WiFiClientSecure _secureClient;
    bool             _isInitialized;

    bool decodeBase64(const String& input, uint8_t* output, size_t* outLen, size_t maxLen);
};

extern TTSClient ttsClient;
