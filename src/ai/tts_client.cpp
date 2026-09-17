#include "tts_client.h"
#include "audio/speaker_driver.h"
#include <WiFi.h>
#include <HTTPClient.h>

TTSClient ttsClient;

TTSClient::TTSClient() : _isInitialized(false) {}

TTSClient::~TTSClient() {}

bool TTSClient::begin() {
    _secureClient.setInsecure();
    _isInitialized = true;
    return true;
}

static inline int b64_char_to_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool TTSClient::decodeBase64(const String& input, uint8_t* output, size_t* outLen, size_t maxLen) {
    if (!output || !outLen) return false;
    *outLen = 0;

    int len = input.length();
    int i = 0;

    while (i < len) {
        if (input[i] == '\r' || input[i] == '\n' || input[i] == ' ' || input[i] == '\t') {
            i++;
            continue;
        }

        char c0 = input[i++];
        char c1 = (i < len) ? input[i++] : '=';
        char c2 = (i < len) ? input[i++] : '=';
        char c3 = (i < len) ? input[i++] : '=';

        int v0 = b64_char_to_val(c0);
        int v1 = b64_char_to_val(c1);
        int v2 = (c2 != '=') ? b64_char_to_val(c2) : 0;
        int v3 = (c3 != '=') ? b64_char_to_val(c3) : 0;

        if (v0 < 0 || v1 < 0) break;

        if (*outLen < maxLen) output[(*outLen)++] = (uint8_t)((v0 << 2) | (v1 >> 4));
        if (c2 != '=' && *outLen < maxLen) output[(*outLen)++] = (uint8_t)(((v1 & 0x0F) << 4) | (v2 >> 2));
        if (c3 != '=' && *outLen < maxLen) output[(*outLen)++] = (uint8_t)(((v2 & 0x03) << 6) | v3);
    }
    return (*outLen > 0);
}

static String urlEncode(const String& str) {
    String encoded = "";
    char hex[4];
    for (size_t i = 0; i < str.length(); i++) {
        uint8_t c = (uint8_t)str[i];
        if (isalnum((char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += (char)c;
        } else if (c == ' ') {
            encoded += "%20";
        } else {
            sprintf(hex, "%%%02X", c);
            encoded += hex;
        }
    }
    return encoded;
}

// -------------------------------------------------------------------------
// 1. Google Cloud Text-to-Speech Streaming (Studio Neural Human Voice)
// -------------------------------------------------------------------------
bool TTSClient::speakGoogleCloudTTS(const String& text, const String& langCode) {
    if (text.length() == 0) return false;

    String apiKey = String(GOOGLE_TTS_API_KEY);
    apiKey.trim();
    if (apiKey.length() == 0 || apiKey.startsWith("YOUR_")) {
        log_w("Google Cloud TTS API Key is not set.");
        return false;
    }

    String voiceName = (langCode == "en") ? TTS_VOICE_ENGLISH : TTS_VOICE_BANGLA;
    String voiceLang = (langCode == "en") ? "en-IN" : "bn-IN";
    uint32_t sampleRate = 24000;

    JsonDocument doc;
    JsonObject input = doc["input"].to<JsonObject>();
    input["text"] = text;

    JsonObject voice = doc["voice"].to<JsonObject>();
    voice["languageCode"] = voiceLang;
    voice["name"] = voiceName;

    JsonObject audioConfig = doc["audioConfig"].to<JsonObject>();
    audioConfig["audioEncoding"] = "LINEAR16";
    audioConfig["sampleRateHertz"] = sampleRate;
    audioConfig["speakingRate"] = 1.05;
    audioConfig["pitch"] = 2.0;

    String jsonBody;
    serializeJson(doc, jsonBody);

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15);

    log_i("Connecting to Google Cloud TTS for [%s]...", voiceLang.c_str());
    if (!client.connect(GOOGLE_TTS_HOST, 443)) {
        log_e("Failed to connect to Google TTS host (%s:443)", GOOGLE_TTS_HOST);
        return false;
    }

    String path = "/v1/text:synthesize?key=" + apiKey;
    client.print("POST " + path + " HTTP/1.1\r\n");
    client.print("Host: " + String(GOOGLE_TTS_HOST) + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(jsonBody.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(jsonBody);

    // Wait for response status line
    uint32_t startWait = millis();
    while (!client.available() && client.connected() && (millis() - startWait < 10000)) {
        delay(10);
    }

    if (!client.available()) {
        log_e("TTS response timeout!");
        client.stop();
        return false;
    }

    String statusLine = client.readStringUntil('\n');
    int statusCode = 0;
    int firstSpace = statusLine.indexOf(' ');
    if (firstSpace > 0) {
        statusCode = statusLine.substring(firstSpace + 1, firstSpace + 4).toInt();
    }

    if (statusCode != 200) {
        log_w("Google Cloud TTS returned HTTP %d (%s). Please click [Enable] on Google Cloud Console.", statusCode, statusLine.c_str());
        client.stop();
        return false;
    }

    // Skip HTTP headers
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) break;
    }

    // Find "audioContent": " in JSON stream
    log_i("Locating audioContent in TTS stream...");
    const char* targetKey = "\"audioContent\":";
    size_t keyMatchIdx = 0;
    size_t keyLen = strlen(targetKey);
    bool foundKey = false;

    startWait = millis();
    while ((client.connected() || client.available()) && (millis() - startWait < 10000)) {
        if (!client.available()) {
            delay(5);
            continue;
        }
        char c = client.read();
        if (c == targetKey[keyMatchIdx]) {
            keyMatchIdx++;
            if (keyMatchIdx == keyLen) {
                foundKey = true;
                break;
            }
        } else {
            keyMatchIdx = (c == targetKey[0]) ? 1 : 0;
        }
    }

    if (!foundKey) {
        log_e("Could not find audioContent in TTS response!");
        client.stop();
        return false;
    }

    // Skip spaces and opening quote
    while (client.connected() || client.available()) {
        if (client.available()) {
            char c = client.read();
            if (c == '"') break;
        } else {
            delay(2);
        }
    }

    log_i("Streaming Cloud TTS audio directly to MAX98357A speaker...");
    speakerDriver.beginStreaming(sampleRate);

    // Stream-decode Base64 to I2S
    char b64Quad[4];
    int quadPos = 0;
    uint8_t rawByteBuf[256];
    size_t rawByteCount = 0;
    size_t totalBytesSkipped = 0;
    const size_t WAV_HEADER_LEN = 44;
    bool playbackOccurred = false;

    int16_t sampleBuffer[128];
    size_t sampleCount = 0;

    auto flushSamples = [&]() {
        if (sampleCount > 0) {
            speakerDriver.playPCMChunk(sampleBuffer, sampleCount);
            sampleCount = 0;
            playbackOccurred = true;
        }
    };

    auto processRawBytes = [&](const uint8_t* bytes, size_t count) {
        for (size_t b = 0; b < count; b++) {
            if (totalBytesSkipped < WAV_HEADER_LEN) {
                totalBytesSkipped++;
                continue;
            }
            rawByteBuf[rawByteCount++] = bytes[b];
            if (rawByteCount == 2) {
                int16_t s = (int16_t)((uint16_t)rawByteBuf[0] | ((uint16_t)rawByteBuf[1] << 8));
                rawByteCount = 0;
                sampleBuffer[sampleCount++] = s;
                if (sampleCount >= 128) {
                    flushSamples();
                }
            }
        }
    };

    startWait = millis();
    while ((client.connected() || client.available()) && (millis() - startWait < 15000)) {
        if (!client.available()) {
            delay(2);
            continue;
        }
        char c = client.read();
        startWait = millis();

        if (c == '"' || c == '}') {
            break;
        }
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
            continue;
        }

        b64Quad[quadPos++] = c;
        if (quadPos == 4) {
            int v0 = b64_char_to_val(b64Quad[0]);
            int v1 = b64_char_to_val(b64Quad[1]);
            int v2 = (b64Quad[2] != '=') ? b64_char_to_val(b64Quad[2]) : 0;
            int v3 = (b64Quad[3] != '=') ? b64_char_to_val(b64Quad[3]) : 0;

            if (v0 >= 0 && v1 >= 0) {
                uint8_t outBytes[3];
                size_t outLen = 0;
                outBytes[outLen++] = (uint8_t)((v0 << 2) | (v1 >> 4));
                if (b64Quad[2] != '=') outBytes[outLen++] = (uint8_t)(((v1 & 0x0F) << 4) | (v2 >> 2));
                if (b64Quad[3] != '=') outBytes[outLen++] = (uint8_t)(((v2 & 0x03) << 6) | v3);

                processRawBytes(outBytes, outLen);
            }
            quadPos = 0;
        }
    }

    flushSamples();
    speakerDriver.endStreaming();
    client.stop();

    log_i("Cloud TTS streaming finished (Success: %s).", playbackOccurred ? "YES" : "NO");
    return playbackOccurred;
}

// -------------------------------------------------------------------------
// 2. VoiceRSS Free Human Voice (Direct 16kHz WAV Streaming)
// -------------------------------------------------------------------------
bool TTSClient::speakVoiceRSS(const String& text, const String& langCode) {
    String apiKey = String(VOICERSS_API_KEY);
    apiKey.trim();
    if (apiKey.length() == 0 || apiKey.startsWith("YOUR_")) {
        return false;
    }

    String hl = (langCode == "en") ? "en-us" : "bn-bd";
    String url = "http://api.voicerss.org/?key=" + apiKey + "&hl=" + hl + "&c=WAV&f=16khz_16bit_mono&src=" + urlEncode(text);

    log_i("Calling VoiceRSS Free Human Voice for [%s]...", hl.c_str());

    HTTPClient http;
    http.begin(url);
    http.setTimeout(12000);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        log_w("VoiceRSS returned HTTP %d", httpCode);
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    if (!stream) {
        http.end();
        return false;
    }

    speakerDriver.beginStreaming(16000);

    // Skip 44-byte WAV header
    uint8_t header[44];
    size_t headerRead = 0;
    while (headerRead < 44 && stream->connected()) {
        if (stream->available()) {
            header[headerRead++] = stream->read();
        } else {
            delay(2);
        }
    }

    // Stream 16-bit PCM samples directly to speaker
    const size_t CHUNK = 128;
    int16_t pcmBuf[CHUNK];
    uint8_t byteBuf[CHUNK * 2];
    bool playbackOccurred = false;

    uint32_t lastDataTime = millis();
    while (http.connected() || stream->available()) {
        size_t available = stream->available();
        if (available >= 2) {
            lastDataTime = millis();
            size_t bytesToRead = (available > sizeof(byteBuf)) ? sizeof(byteBuf) : available;
            bytesToRead &= ~1; // Ensure 2-byte alignment
            size_t bytesRead = stream->readBytes((char*)byteBuf, bytesToRead);
            size_t samplesCount = bytesRead / sizeof(int16_t);

            for (size_t i = 0; i < samplesCount; i++) {
                int32_t sample = (int16_t)((uint16_t)byteBuf[i * 2] | ((uint16_t)byteBuf[i * 2 + 1] << 8));
                sample = sample * 2; // Boost volume by 200%
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                pcmBuf[i] = (int16_t)sample;
            }

            speakerDriver.playPCMChunk(pcmBuf, samplesCount);
            playbackOccurred = true;
        } else {
            if (millis() - lastDataTime > 4000) {
                log_w("VoiceRSS stream timeout");
                break;
            }
            delay(2);
        }
    }

    speakerDriver.endStreaming();
    http.end();
    return playbackOccurred;
}

// -------------------------------------------------------------------------
// Unified Speak API: Cloud TTS -> VoiceRSS -> Mascot Synth Fallback
// -------------------------------------------------------------------------
bool TTSClient::speakText(const String& text, const String& langCode) {
    if (text.length() == 0) return false;

    // 1. Try Google Cloud TTS
    if (speakGoogleCloudTTS(text, langCode)) {
        return true;
    }

    // 2. Try VoiceRSS Free Human Voice if key provided
    if (speakVoiceRSS(text, langCode)) {
        return true;
    }

    return false;
}

bool TTSClient::synthesize(const String& text, const String& langCode, uint8_t** outPCM, size_t* outPCMSize, uint32_t* outSampleRate) {
    if (!outPCM || !outPCMSize || text.length() == 0) return false;
    *outPCM = nullptr;
    *outPCMSize = 0;
    *outSampleRate = 24000;
    return speakText(text, langCode);
}
