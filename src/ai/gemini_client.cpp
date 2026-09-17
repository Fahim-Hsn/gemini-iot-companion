#include "gemini_client.h"
#include <WiFiClientSecure.h>

GeminiClient geminiClient;

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

GeminiClient::GeminiClient() : _isInitialized(false) {
    _profile.userName = DEFAULT_USER_NAME;
    _profile.mascotName = MASCOT_NAME;
    _profile.playfulness = 0.85f;
}

GeminiClient::~GeminiClient() {}

bool GeminiClient::begin() {
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

static String readHttpResponsePayload(WiFiClientSecure& client, bool isChunked, int contentLength) {
    String payload = "";

    if (isChunked) {
        log_i("Reading chunked HTTP response from Gemini...");
        uint32_t startWait = millis();
        while ((client.connected() || client.available()) && (millis() - startWait < 15000)) {
            String sizeLine = client.readStringUntil('\n');
            sizeLine.trim();
            if (sizeLine.length() == 0) continue;

            size_t chunkSize = strtoul(sizeLine.c_str(), NULL, 16);
            if (chunkSize == 0) {
                // End of chunked stream
                break;
            }

            size_t bytesRead = 0;
            while (bytesRead < chunkSize && (client.connected() || client.available())) {
                if (client.available()) {
                    char c = client.read();
                    payload += c;
                    bytesRead++;
                } else {
                    delay(2);
                }
            }
            // Read trailing CRLF
            client.readStringUntil('\n');
        }
    } else if (contentLength > 0) {
        log_i("Reading %d bytes HTTP response from Gemini...", contentLength);
        payload.reserve(contentLength + 16);
        uint32_t startWait = millis();
        while ((int)payload.length() < contentLength && (millis() - startWait < 15000)) {
            while (client.available()) {
                payload += client.readString();
            }
            delay(10);
        }
    } else {
        // Fallback: Read until EOF / connection closed
        uint32_t startWait = millis();
        while ((client.connected() || client.available()) && (millis() - startWait < 15000)) {
            while (client.available()) {
                payload += client.readString();
            }
            delay(10);
        }
    }

    log_i("HTTP Payload complete (%u bytes).", (unsigned)payload.length());
    return payload;
}

bool GeminiClient::processAudioQuery(const uint8_t* pcmAudio, size_t audioSize, AIResponse* outResponse) {
    if (!pcmAudio || audioSize == 0 || !outResponse) return false;

    // Cap audio size to 32KB (1.0 sec) for rock-solid stability
    if (audioSize > 32000) {
        audioSize = 32000;
    }

    log_i("Preparing Gemini voice query (%u bytes)... Free Heap: %u, Max Alloc: %u", 
          (unsigned)audioSize, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());

    // 1. Build standard 44-byte WAV header
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

    size_t totalWavBytes = 44 + audioSize;
    size_t base64Len = ((totalWavBytes + 2) / 3) * 4;

    auto getWavByte = [&](size_t idx) -> uint8_t {
        if (idx < 44) return wavHeader[idx];
        return pcmAudio[idx - 44];
    };

    // System instruction string
    String sysText = buildSystemInstruction();
    String escapedSys = "";
    escapedSys.reserve(sysText.length() + 32);
    for (size_t i = 0; i < sysText.length(); i++) {
        char c = sysText[i];
        if (c == '"') escapedSys += "\\\"";
        else if (c == '\\') escapedSys += "\\\\";
        else if (c == '\n') escapedSys += "\\n";
        else if (c == '\r') escapedSys += "\\r";
        else if (c == '\t') escapedSys += "\\t";
        else escapedSys += c;
    }

    String jsonPrefix = "{\"system_instruction\":{\"parts\":[{\"text\":\"" + escapedSys + "\"}]},\"contents\":[{\"role\":\"user\",\"parts\":[{\"inline_data\":{\"mime_type\":\"audio/wav\",\"data\":\"";
    String jsonSuffix = "\"}},{\"text\":\"Listen to my voice audio and respond according to instructions.\"}]}]}";

    size_t totalPayloadLen = jsonPrefix.length() + base64Len + jsonSuffix.length();

    // 2. Open Secure TLS Socket directly with SNI Hostname
    WiFiClientSecure client;
    client.setInsecure(); // Skip cert chain for embedded speed
    client.setTimeout(20);

    log_i("Connecting TLS to %s:443 ... Free Heap: %u", GEMINI_API_HOST, (unsigned)ESP.getFreeHeap());

    if (!client.connect(GEMINI_API_HOST, 443)) {
        log_e("TLS socket connect failed to %s:443", GEMINI_API_HOST);
        outResponse->isSuccess = false;
        outResponse->errorMessage = "TLS connect Fail";
        return false;
    }

    log_i("TLS Connected! Streaming HTTP POST (%u bytes)...", (unsigned)totalPayloadLen);

    String path = "/v1beta/models/" + String(GEMINI_MODEL) + ":generateContent?key=" + String(GEMINI_API_KEY);
    client.print("POST " + path + " HTTP/1.1\r\n");
    client.print("Host: " + String(GEMINI_API_HOST) + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(totalPayloadLen) + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // Send JSON Prefix
    client.print(jsonPrefix);

    // Stream Base64 audio directly in small 768-byte chunks without allocating strings
    char b64Chunk[769];
    size_t wavPos = 0;
    while (wavPos < totalWavBytes && client.connected()) {
        size_t chunkRaw = 0;
        size_t chunkOut = 0;
        while (chunkRaw < 576 && wavPos < totalWavBytes) {
            uint32_t octet_a = (wavPos < totalWavBytes) ? getWavByte(wavPos++) : 0;
            uint32_t octet_b = (wavPos < totalWavBytes) ? getWavByte(wavPos++) : 0;
            uint32_t octet_c = (wavPos < totalWavBytes) ? getWavByte(wavPos++) : 0;
            chunkRaw += 3;

            uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

            b64Chunk[chunkOut++] = b64_table[(triple >> 18) & 0x3F];
            b64Chunk[chunkOut++] = b64_table[(triple >> 12) & 0x3F];
            b64Chunk[chunkOut++] = (wavPos > totalWavBytes + 1) ? '=' : b64_table[(triple >> 6) & 0x3F];
            b64Chunk[chunkOut++] = (wavPos > totalWavBytes) ? '=' : b64_table[triple & 0x3F];
        }
        b64Chunk[chunkOut] = '\0';
        client.write((const uint8_t*)b64Chunk, chunkOut);
    }

    // Send JSON Suffix
    client.print(jsonSuffix);

    log_i("Payload streamed. Waiting for Gemini response...");

    // Read HTTP Status Line
    uint32_t startWait = millis();
    while (!client.available() && client.connected() && (millis() - startWait < 20000)) {
        delay(10);
    }

    if (!client.available()) {
        log_e("Gemini response timeout!");
        client.stop();
        outResponse->isSuccess = false;
        outResponse->errorMessage = "Response Timeout";
        return false;
    }

    String statusLine = client.readStringUntil('\n');
    log_i("HTTP Status: %s", statusLine.c_str());

    int statusCode = 0;
    int firstSpace = statusLine.indexOf(' ');
    if (firstSpace > 0) {
        statusCode = statusLine.substring(firstSpace + 1, firstSpace + 4).toInt();
    }

    // Parse HTTP Headers
    bool isChunked = false;
    int contentLength = -1;
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) {
            break; // Headers ended
        }
        String lowerLine = line;
        lowerLine.toLowerCase();
        if (lowerLine.startsWith("transfer-encoding:") && lowerLine.indexOf("chunked") >= 0) {
            isChunked = true;
        }
        if (lowerLine.startsWith("content-length:")) {
            contentLength = line.substring(15).toInt();
        }
    }

    // Read decoded Response Payload
    String responsePayload = readHttpResponsePayload(client, isChunked, contentLength);
    client.stop();

    if (statusCode == 200) {
        return parseStructuredResponse(responsePayload, outResponse);
    } else {
        log_e("Gemini API Error (HTTP %d): %s", statusCode, responsePayload.c_str());
        outResponse->isSuccess = false;
        outResponse->errorMessage = "HTTP " + String(statusCode);
        return false;
    }
}

bool GeminiClient::sendGeminiRequest(const String& jsonBody, AIResponse* outResponse) {
    if (!outResponse) return false;

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(20);

    log_i("Connecting TLS to %s:443 ... Free Heap: %u", GEMINI_API_HOST, (unsigned)ESP.getFreeHeap());

    if (!client.connect(GEMINI_API_HOST, 443)) {
        log_e("TLS socket connect failed to %s:443", GEMINI_API_HOST);
        outResponse->isSuccess = false;
        outResponse->errorMessage = "TLS connect Fail";
        return false;
    }

    log_i("TLS Socket connected! Sending HTTP POST...");

    String path = "/v1beta/models/" + String(GEMINI_MODEL) + ":generateContent?key=" + String(GEMINI_API_KEY);
    
    // Send HTTP Headers
    client.print("POST " + path + " HTTP/1.1\r\n");
    client.print("Host: " + String(GEMINI_API_HOST) + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(jsonBody.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // Stream Payload in 1KB chunks
    const char* buf = jsonBody.c_str();
    size_t remaining = jsonBody.length();
    size_t offset = 0;
    while (remaining > 0 && client.connected()) {
        size_t toWrite = (remaining > 1024) ? 1024 : remaining;
        size_t written = client.write((const uint8_t*)(buf + offset), toWrite);
        if (written == 0) break;
        offset += written;
        remaining -= written;
    }

    log_i("Payload sent (%u bytes). Waiting for Gemini response...", (unsigned)jsonBody.length());

    // Read HTTP Status Line
    uint32_t startWait = millis();
    while (!client.available() && client.connected() && (millis() - startWait < 20000)) {
        delay(10);
    }

    if (!client.available()) {
        log_e("Gemini response timeout!");
        client.stop();
        outResponse->isSuccess = false;
        outResponse->errorMessage = "Response Timeout";
        return false;
    }

    String statusLine = client.readStringUntil('\n');
    log_i("HTTP Status: %s", statusLine.c_str());

    // Parse status code (e.g. "HTTP/1.1 200 OK")
    int statusCode = 0;
    int firstSpace = statusLine.indexOf(' ');
    if (firstSpace > 0) {
        statusCode = statusLine.substring(firstSpace + 1, firstSpace + 4).toInt();
    }

    // Parse HTTP Headers
    bool isChunked = false;
    int contentLength = -1;
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) {
            break; // Headers ended
        }
        String lowerLine = line;
        lowerLine.toLowerCase();
        if (lowerLine.startsWith("transfer-encoding:") && lowerLine.indexOf("chunked") >= 0) {
            isChunked = true;
        }
        if (lowerLine.startsWith("content-length:")) {
            contentLength = line.substring(15).toInt();
        }
    }

    // Read JSON response payload
    String responsePayload = readHttpResponsePayload(client, isChunked, contentLength);
    client.stop();

    if (statusCode == 200) {
        return parseStructuredResponse(responsePayload, outResponse);
    } else {
        log_e("Gemini API Error (HTTP %d): %s", statusCode, responsePayload.c_str());
        outResponse->isSuccess = false;
        outResponse->errorMessage = "HTTP " + String(statusCode);
        return false;
    }
}

bool GeminiClient::parseStructuredResponse(const String& responseJson, AIResponse* outResponse) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, responseJson);
    if (error) {
        log_e("Failed to parse Gemini API JSON: %s. Payload: %s", error.c_str(), responseJson.c_str());
        outResponse->isSuccess = false;
        outResponse->errorMessage = "JSON parse error";
        return false;
    }

    // Check for API-level error object
    if (doc["error"].is<JsonObject>()) {
        const char* errMsg = doc["error"]["message"];
        log_e("Gemini API error payload: %s", errMsg ? errMsg : "Unknown");
        outResponse->isSuccess = false;
        outResponse->errorMessage = errMsg ? String(errMsg).substring(0, 18) : "API Error";
        return false;
    }

    // Extract Candidate
    JsonArray candidates = doc["candidates"].as<JsonArray>();
    if (candidates.isNull() || candidates.size() == 0) {
        log_e("No candidates in Gemini response!");
        outResponse->isSuccess = false;
        outResponse->errorMessage = "No Candidate";
        return false;
    }

    const char* textContent = candidates[0]["content"]["parts"][0]["text"];
    if (!textContent) {
        log_e("No text part in Gemini candidate!");
        outResponse->isSuccess = false;
        outResponse->errorMessage = "Empty AI response";
        return false;
    }

    String innerText = String(textContent);
    innerText.trim();

    // Clean markdown formatting if present
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

    // Robust JSON extraction: look for outer braces { ... }
    int firstBrace = innerText.indexOf('{');
    int lastBrace = innerText.lastIndexOf('}');
    bool parsedSuccessfully = false;

    if (firstBrace >= 0 && lastBrace > firstBrace) {
        String jsonCandidate = innerText.substring(firstBrace, lastBrace + 1);
        JsonDocument parsedDoc;
        DeserializationError innerErr = deserializeJson(parsedDoc, jsonCandidate);
        if (!innerErr) {
            outResponse->speechText = parsedDoc["speech"] | "";
            outResponse->languageCode = parsedDoc["lang"] | "bn";
            parsedSuccessfully = true;

            String emotionStr = parsedDoc["emotion"] | "idle";
            if (emotionStr == "happy") outResponse->emotion = MascotEmotion::HAPPY;
            else if (emotionStr == "sad") outResponse->emotion = MascotEmotion::SAD;
            else if (emotionStr == "thinking") outResponse->emotion = MascotEmotion::THINKING;
            else if (emotionStr == "confused") outResponse->emotion = MascotEmotion::CONFUSED;
            else if (emotionStr == "excited") outResponse->emotion = MascotEmotion::EXCITED;
            else if (emotionStr == "sleeping") outResponse->emotion = MascotEmotion::SLEEPING;
            else outResponse->emotion = MascotEmotion::IDLE;

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

            JsonObject home = parsedDoc["smart_home"];
            if (!home.isNull()) {
                outResponse->homeCmd.action = home["action"] | "none";
                outResponse->homeCmd.target = home["target"] | "";
                outResponse->homeCmd.value = home["value"] | 0;
                outResponse->homeCmd.metadata = home["meta"] | "";
                outResponse->homeCmd.isValid = (outResponse->homeCmd.action != "none");
            }
        }
    }

    if (!parsedSuccessfully || outResponse->speechText.length() == 0) {
        log_w("Could not parse structured JSON from text. Using as plain speech.");
        outResponse->speechText = innerText;
        outResponse->languageCode = "bn";
        outResponse->emotion = MascotEmotion::HAPPY;
        outResponse->displayCmd.mode = DisplayMode::SPEECH_BUBBLE;
        outResponse->displayCmd.speechText = innerText;
    }

    outResponse->isSuccess = true;
    log_i("Gemini reply received: [%s] '%s'", outResponse->languageCode.c_str(), outResponse->speechText.c_str());
    return true;
}

