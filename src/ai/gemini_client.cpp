#include "gemini_client.h"
#include <HTTPClient.h>

GeminiClient geminiClient;

// Base64 lookup table
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

GeminiClient::GeminiClient() : _isInitialized(false) {
    _profile.userName = DEFAULT_USER_NAME;
    _profile.mascotName = MASCOT_NAME;
    _profile.playfulness = 0.85f;
}

GeminiClient::~GeminiClient() {}

bool GeminiClient::begin() {
    _secureClient.setInsecure(); // Skip root CA verification for lightweight embedded HTTPS
    _isInitialized = true;
    return true;
}

void GeminiClient::setPersonality(const PersonalityProfile& profile) {
    _profile = profile;
}

String GeminiClient::buildSystemInstruction() {
    String sys = "You are 'Bondhu' (also called 'Kiko'), a very cute, helpful, and lively animal mascot (Kitsune/Fox) desktop AI companion created for " + _profile.userName + ".\n";
    sys += "Guidelines:\n";
    sys += "1. You speak both Bengali (Bangla) and English. If the user speaks Bangla, reply in cute Bangla. If in English, reply in cheerful English.\n";
    sys += "2. Keep your speech concise (1 to 2 sentences max) so it sounds natural on a desktop speaker.\n";
    sys += "3. ALWAYS format your entire response as a valid, single JSON object strictly matching this schema:\n";
    sys += "{\n";
    sys += "  \"speech\": \"Your voice reply text\",\n";
    sys += "  \"lang\": \"bn\" or \"en\",\n";
    sys += "  \"emotion\": \"idle\" | \"happy\" | \"sad\" | \"confused\" | \"thinking\" | \"excited\" | \"sleeping\",\n";
    sys += "  \"display\": {\n";
    sys += "    \"mode\": \"mascot\" | \"clock\" | \"weather\" | \"todo\" | \"card\",\n";
    sys += "    \"title\": \"optional title\",\n";
    sys += "    \"subtitle\": \"optional subtitle\",\n";
    sys += "    \"theme\": \"midnight\" | \"cyberpunk\" | \"pastel\" | \"green\"\n";
    sys += "  },\n";
    sys += "  \"smart_home\": {\n";
    sys += "    \"action\": \"none\" | \"light_on\" | \"light_off\" | \"fan_on\" | \"fan_off\" | \"set_alarm\" | \"set_timer\",\n";
    sys += "    \"target\": \"light\" | \"fan\" | \"alarm\",\n";
    sys += "    \"value\": 0,\n";
    sys += "    \"meta\": \"description\"\n";
    sys += "  }\n";
    sys += "}\n";
    sys += "Do not include markdown code block wrappers (like ```json), just output the raw JSON string.";
    return sys;
}

String GeminiClient::base64EncodeAudio(const uint8_t* data, size_t length) {
    String encoded;
    encoded.reserve(((length + 2) / 3) * 4);

    size_t i = 0;
    while (i < length) {
        uint32_t octet_a = i < length ? data[i++] : 0;
        uint32_t octet_b = i < length ? data[i++] : 0;
        uint32_t octet_c = i < length ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        encoded += b64_table[(triple >> 18) & 0x3F];
        encoded += b64_table[(triple >> 12) & 0x3F];
        encoded += (i > length + 1) ? '=' : b64_table[(triple >> 6) & 0x3F];
        encoded += (i > length) ? '=' : b64_table[triple & 0x3F];
    }
    return encoded;
}

bool GeminiClient::processTextQuery(const String& promptText, AIResponse* outResponse) {
    if (!outResponse) return false;

    JsonDocument doc;
    JsonObject sysInst = doc["system_instruction"].to<JsonObject>();
    JsonObject sysPart = sysInst["parts"].to<JsonArray>().add<JsonObject>();
    sysPart["text"] = buildSystemInstruction();

    JsonObject content = doc["contents"].to<JsonArray>().add<JsonObject>();
    content["role"] = "user";
    JsonObject userPart = content["parts"].to<JsonArray>().add<JsonObject>();
    userPart["text"] = promptText;

    String jsonBody;
    serializeJson(doc, jsonBody);

    return sendGeminiRequest(jsonBody, outResponse);
}

bool GeminiClient::processAudioQuery(const uint8_t* pcmAudio, size_t audioSize, AIResponse* outResponse) {
    if (!pcmAudio || audioSize == 0 || !outResponse) return false;

    log_i("Preparing Gemini multimodal audio payload (%u bytes PCM)...", (unsigned)audioSize);

    // Create a 44-byte WAV header for the raw 16kHz mono PCM
    uint8_t wavHeader[44];
    uint32_t totalDataLen = audioSize;
    uint32_t totalFileLen = totalDataLen + 36;
    uint32_t sampleRate = MIC_SAMPLE_RATE;
    uint16_t numChannels = 1;
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);

    memcpy(wavHeader, "RIFF", 4);
    memcpy(wavHeader + 4, &totalFileLen, 4);
    memcpy(wavHeader + 8, "WAVEfmt ", 8);
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    memcpy(wavHeader + 16, &subchunk1Size, 4);
    memcpy(wavHeader + 20, &audioFormat, 2);
    memcpy(wavHeader + 22, &numChannels, 2);
    memcpy(wavHeader + 24, &sampleRate, 4);
    memcpy(wavHeader + 28, &byteRate, 4);
    memcpy(wavHeader + 32, &blockAlign, 2);
    memcpy(wavHeader + 34, &bitsPerSample, 2);
    memcpy(wavHeader + 36, "data", 4);
    memcpy(wavHeader + 40, &totalDataLen, 4);

    // Allocate combined buffer for Base64 encoding
    size_t totalWavSize = 44 + audioSize;
    uint8_t* fullWav = (uint8_t*)malloc(totalWavSize);
    if (!fullWav) {
        log_e("Out of memory for full WAV packaging!");
        outResponse->errorMessage = "Memory allocation failed";
        return false;
    }

    memcpy(fullWav, wavHeader, 44);
    memcpy(fullWav + 44, pcmAudio, audioSize);

    String base64Audio = base64EncodeAudio(fullWav, totalWavSize);
    free(fullWav);

    JsonDocument doc;
    JsonObject sysInst = doc["system_instruction"].to<JsonObject>();
    JsonObject sysPart = sysInst["parts"].to<JsonArray>().add<JsonObject>();
    sysPart["text"] = buildSystemInstruction();

    JsonObject content = doc["contents"].to<JsonArray>().add<JsonObject>();
    content["role"] = "user";
    JsonArray parts = content["parts"].to<JsonArray>();

    JsonObject audioPart = parts.add<JsonObject>();
    JsonObject inlineData = audioPart["inline_data"].to<JsonObject>();
    inlineData["mime_type"] = "audio/wav";
    inlineData["data"] = base64Audio;

    JsonObject textPart = parts.add<JsonObject>();
    textPart["text"] = "Listen to my voice audio and respond according to instructions.";

    String jsonBody;
    serializeJson(doc, jsonBody);

    return sendGeminiRequest(jsonBody, outResponse);
}

bool GeminiClient::sendGeminiRequest(const String& jsonBody, AIResponse* outResponse) {
    HTTPClient http;
    String url = "https://" + String(GEMINI_API_HOST) + "/v1beta/models/" + String(GEMINI_MODEL) + ":generateContent?key=" + String(GEMINI_API_KEY);

    http.begin(_secureClient, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(18000);

    log_i("Sending request to Gemini (%s)...", GEMINI_MODEL);
    int httpCode = http.POST(jsonBody);

    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        String responsePayload = http.getString();
        http.end();
        return parseStructuredResponse(responsePayload, outResponse);
    } else {
        String err = http.getString();
        log_e("Gemini API Error (%d): %s", httpCode, err.c_str());
        outResponse->isSuccess = false;
        outResponse->errorMessage = "Gemini Error: " + String(httpCode);
        http.end();
        return false;
    }
}

bool GeminiClient::parseStructuredResponse(const String& responseJson, AIResponse* outResponse) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, responseJson);
    if (error) {
        log_e("Failed to parse Gemini API JSON: %s", error.c_str());
        outResponse->isSuccess = false;
        outResponse->errorMessage = "JSON parse error";
        return false;
    }

    const char* textContent = doc["candidates"][0]["content"]["parts"][0]["text"];
    if (!textContent) {
        log_e("No text part in Gemini candidate!");
        outResponse->isSuccess = false;
        outResponse->errorMessage = "Empty AI response";
        return false;
    }

    String innerText = String(textContent);
    innerText.trim();

    // Clean any markdown formatting if present
    if (innerText.startsWith("```json")) {
        innerText = innerText.substring(7);
    } else if (innerText.startsWith("```")) {
        innerText = innerText.substring(3);
    }
    if (innerText.endsWith("```")) {
        innerText = innerText.substring(0, innerText.length() - 3);
    }
    innerText.trim();

    outResponse->rawResponseText = innerText;

    // Parse structured JSON payload
    JsonDocument parsedDoc;
    DeserializationError innerErr = deserializeJson(parsedDoc, innerText);
    if (innerErr) {
        log_w("Could not parse structured JSON from text. Using as plain speech.");
        outResponse->speechText = innerText;
        outResponse->languageCode = "bn";
        outResponse->emotion = MascotEmotion::HAPPY;
        outResponse->displayCmd.mode = DisplayMode::SPEECH_BUBBLE;
        outResponse->displayCmd.speechText = innerText;
        outResponse->isSuccess = true;
        return true;
    }

    outResponse->speechText = parsedDoc["speech"].as<String>();
    outResponse->languageCode = parsedDoc["lang"] | "bn";

    String emotionStr = parsedDoc["emotion"] | "idle";
    if (emotionStr == "happy") outResponse->emotion = MascotEmotion::HAPPY;
    else if (emotionStr == "sad") outResponse->emotion = MascotEmotion::SAD;
    else if (emotionStr == "thinking") outResponse->emotion = MascotEmotion::THINKING;
    else if (emotionStr == "confused") outResponse->emotion = MascotEmotion::CONFUSED;
    else if (emotionStr == "excited") outResponse->emotion = MascotEmotion::EXCITED;
    else if (emotionStr == "sleeping") outResponse->emotion = MascotEmotion::SLEEPING;
    else outResponse->emotion = MascotEmotion::IDLE;

    // Display command
    JsonObject disp = parsedDoc["display"];
    if (!disp.isNull()) {
        String modeStr = disp["mode"] | "mascot";
        if (modeStr == "clock") outResponse->displayCmd.mode = DisplayMode::CLOCK_OVERLAY;
        else if (modeStr == "weather") outResponse->displayCmd.mode = DisplayMode::WEATHER_OVERLAY;
        else if (modeStr == "todo") outResponse->displayCmd.mode = DisplayMode::TODO_LIST;
        else if (modeStr == "card") outResponse->displayCmd.mode = DisplayMode::CUSTOM_TEXT;
        else outResponse->displayCmd.mode = DisplayMode::SPEECH_BUBBLE;

        outResponse->displayCmd.title = disp["title"] | "";
        outResponse->displayCmd.subtitle = disp["subtitle"] | "";
        outResponse->displayCmd.speechText = outResponse->speechText;
        outResponse->displayCmd.durationMs = 6000;
        outResponse->displayCmd.isValid = true;

        String themeStr = disp["theme"] | "midnight";
        if (themeStr == "cyberpunk") outResponse->displayCmd.theme = UITheme::CYBERPUNK_DARK;
        else if (themeStr == "pastel") outResponse->displayCmd.theme = UITheme::NEKO_PASTEL;
        else if (themeStr == "green") outResponse->displayCmd.theme = UITheme::NATURE_GREEN;
        else outResponse->displayCmd.theme = UITheme::MIDNIGHT_FOX;
    }

    // Smart home command
    JsonObject home = parsedDoc["smart_home"];
    if (!home.isNull()) {
        outResponse->homeCmd.action = home["action"] | "none";
        outResponse->homeCmd.target = home["target"] | "";
        outResponse->homeCmd.value = home["value"] | 0;
        outResponse->homeCmd.metadata = home["meta"] | "";
        outResponse->homeCmd.isValid = (outResponse->homeCmd.action != "none");
    }

    outResponse->isSuccess = true;
    log_i("Gemini reply received: [%s] '%s'", outResponse->languageCode.c_str(), outResponse->speechText.c_str());
    return true;
}
