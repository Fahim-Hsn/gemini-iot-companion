#include "tts_client.h"
#include "audio/speaker_driver.h"
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
        if (input[i] == '\r' || input[i] == '\n' || input[i] == ' ') {
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

bool TTSClient::synthesize(const String& text, const String& langCode, uint8_t** outPCM, size_t* outPCMSize, uint32_t* outSampleRate) {
    if (!outPCM || !outPCMSize || text.length() == 0) return false;

    *outPCM = nullptr;
    *outPCMSize = 0;
    *outSampleRate = 24000;

    // Pick voice configuration based on language
    String voiceName = (langCode == "en") ? TTS_VOICE_ENGLISH : TTS_VOICE_BANGLA;
    String voiceLang = (langCode == "en") ? "en-IN" : "bn-IN";

    JsonDocument doc;
    JsonObject input = doc["input"].to<JsonObject>();
    input["text"] = text;

    JsonObject voice = doc["voice"].to<JsonObject>();
    voice["languageCode"] = voiceLang;
    voice["name"] = voiceName;

    JsonObject audioConfig = doc["audioConfig"].to<JsonObject>();
    audioConfig["audioEncoding"] = "LINEAR16";
    audioConfig["sampleRateHertz"] = *outSampleRate;
    audioConfig["speakingRate"] = 1.05; // Slightly lively rate
    audioConfig["pitch"] = 3.0;         // Slightly higher pitch for cute mascot tone

    String jsonBody;
    serializeJson(doc, jsonBody);

    HTTPClient http;
    String url = "https://" + String(GOOGLE_TTS_HOST) + "/v1/text:synthesize?key=" + String(GOOGLE_TTS_API_KEY);

    http.begin(_secureClient, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(12000);

    log_i("Calling Google Cloud TTS for [%s]...", voiceLang.c_str());
    int httpCode = http.POST(jsonBody);

    if (httpCode == 200) {
        String resp = http.getString();
        http.end();

        JsonDocument respDoc;
        deserializeJson(respDoc, resp);
        const char* audioContent = respDoc["audioContent"];
        if (!audioContent) {
            log_e("No audioContent in TTS response!");
            return false;
        }

        String b64Audio = String(audioContent);
        size_t approxLen = (b64Audio.length() * 3) / 4;

        // Allocate buffer in PSRAM/Heap
        uint8_t* pcmBuffer = (uint8_t*)malloc(approxLen + 1024);
        if (!pcmBuffer) {
            log_e("Failed to allocate memory for TTS audio buffer!");
            return false;
        }

        size_t written = 0;
        if (decodeBase64(b64Audio, pcmBuffer, &written, approxLen + 1024)) {
            *outPCM = pcmBuffer;
            *outPCMSize = written;
            log_i("TTS Synthesized successfully (%u bytes PCM).", (unsigned)written);
            return true;
        } else {
            free(pcmBuffer);
            log_e("Failed to decode Base64 TTS audio!");
            return false;
        }
    } else {
        log_e("TTS HTTP Error (%d): %s", httpCode, http.getString().c_str());
        http.end();
        return false;
    }
}

bool TTSClient::speakText(const String& text, const String& langCode) {
    uint8_t* pcm = nullptr;
    size_t pcmSize = 0;
    uint32_t sampleRate = 24000;

    if (synthesize(text, langCode, &pcm, &pcmSize, &sampleRate)) {
        if (pcm && pcmSize > 0) {
            speakerDriver.playPCM(pcm, pcmSize, sampleRate, true);
            free(pcm);
            return true;
        }
    }
    return false;
}
