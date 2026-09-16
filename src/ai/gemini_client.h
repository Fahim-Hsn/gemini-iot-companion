#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "types.h"
#include "config.h"

class GeminiClient {
public:
    GeminiClient();
    ~GeminiClient();

    bool begin();
    
    // Query Gemini using Recorded Raw Audio PCM (16kHz 16-bit Mono)
    bool processAudioQuery(const uint8_t* pcmAudio, size_t audioSize, AIResponse* outResponse);

    // Query Gemini using Text String
    bool processTextQuery(const String& promptText, AIResponse* outResponse);

    // Set custom persona details
    void setPersonality(const PersonalityProfile& profile);

private:
    WiFiClientSecure   _secureClient;
    PersonalityProfile _profile;
    bool               _isInitialized;

    String buildSystemInstruction();
    bool sendGeminiRequest(const String& jsonBody, AIResponse* outResponse);
    bool parseStructuredResponse(const String& responseJson, AIResponse* outResponse);
    String base64EncodeAudio(const uint8_t* data, size_t length);
};

extern GeminiClient geminiClient;
