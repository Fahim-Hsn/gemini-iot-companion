#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "display/st7789_driver.h"
#include "display/animation_engine.h"
#include "display/ui_manager.h"
#include "audio/mic_driver.h"
#include "audio/speaker_driver.h"
#include "ai/gemini_client.h"
#include "ai/tts_client.h"
#include "ai/context_manager.h"
#include "smart_home/home_controller.h"
#include "system/wifi_manager.h"
#include "system/ntp_client.h"
#include "system/weather_client.h"

// System state tracking
static volatile SystemState currentSystemState = SystemState::BOOTING;
static volatile bool triggerVoiceChat = false;

// Audio buffer in internal RAM: 1 second of 16kHz 16-bit mono audio (32 KB)
#define AUDIO_BUF_SIZE (MIC_SAMPLE_RATE * sizeof(int16_t) * 1)
static uint8_t* voiceRecordBuffer = nullptr;

// FreeRTOS Task Handles
TaskHandle_t hDisplayTask = NULL;
TaskHandle_t hVoiceTask = NULL;
TaskHandle_t hSystemTask = NULL;

// ISR for Button Action
void IRAM_ATTR onActionButtonPressed() {
    triggerVoiceChat = true;
}

// -------------------------------------------------------------------------
// Display Animation FreeRTOS Task (30 FPS Butter-Smooth Zero-Flicker)
// -------------------------------------------------------------------------
void displayTaskFunc(void* parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(33); // 30 FPS

    log_i("Display Task running on Core %d", xPortGetCoreID());

    while (true) {
        float mouthLevel = speakerDriver.getCurrentMouthLevel();

        // 1. Update mascot physics & internal timers
        animationEngine.update(mouthLevel);

        // 2. Render mascot into offscreen buffer (zero screen flicker)
        animationEngine.render(displayDriver.getMascotSprite(), mouthLevel);

        // 3. Instant DMA blit of mascot sprite to center of screen
        displayDriver.pushMascotSprite((LCD_WIDTH - SPRITE_W) / 2, 22);

        // 4. Update status & dialogue overlays
        uiManager.renderOverlay(displayDriver.getLGFX());

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// -------------------------------------------------------------------------
// Voice Interaction Pipeline (Record -> Gemini API -> TTS -> Speaker)
// -------------------------------------------------------------------------
void voiceTaskFunc(void* parameter) {
    log_i("Voice Task running on Core %d", xPortGetCoreID());

    while (true) {
        // Wait for voice activation trigger (Button or Action)
        if (!triggerVoiceChat) {
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }
        triggerVoiceChat = false;

        log_i("--- Starting Voice Conversation Turn ---");
        currentSystemState = SystemState::LISTENING_VOICE;
        uiManager.setSystemStatusText("Listening...");
        animationEngine.setEmotion(MascotEmotion::LISTENING);
        
        // Play wake chime on speaker
        speakerDriver.begin(SPK_SAMPLE_RATE);
        speakerDriver.playWakeChime();

        // Verify WiFi is connected
        if (!wifiManager.isConnected()) {
            log_w("Cannot process voice: WiFi is not connected!");
            speakerDriver.begin(SPK_SAMPLE_RATE);
            speakerDriver.playErrorTone();
            uiManager.setCardInfo("WiFi Offline", "Please check router", 4000);
            animationEngine.setEmotion(MascotEmotion::CONFUSED, 3000);
            uiManager.setSystemStatusText("WiFi Offline");
            currentSystemState = SystemState::STANDBY_IDLE;
            continue;
        }

        // 1. Record voice from INMP441 Microphone (I2S RX Mode)
        speakerDriver.end(); // Release I2S bus for mic input
        micDriver.begin();

        size_t recordedBytes = 0;
        if (voiceRecordBuffer) {
            micDriver.startRecording(voiceRecordBuffer, AUDIO_BUF_SIZE, &recordedBytes, 2500);
        }
        micDriver.end(); // Done recording

        // Re-enable speaker for feedback tones & TTS
        speakerDriver.begin(SPK_SAMPLE_RATE);

        if (recordedBytes < (MIC_SAMPLE_RATE * sizeof(int16_t) / 4)) {
            log_w("No valid voice audio recorded.");
            speakerDriver.playErrorTone();
            animationEngine.setEmotion(MascotEmotion::CONFUSED, 2000);
            uiManager.setSystemStatusText("Ready");
            currentSystemState = SystemState::STANDBY_IDLE;
            continue;
        }

        // 2. Processing with Gemini 3.6 Flash API
        currentSystemState = SystemState::CALLING_GEMINI;
        uiManager.setSystemStatusText("Thinking...");
        animationEngine.setEmotion(MascotEmotion::THINKING);
        speakerDriver.playReadyBeep();

        AIResponse aiResp;
        bool geminiSuccess = geminiClient.processAudioQuery(voiceRecordBuffer, recordedBytes, &aiResp);

        if (!geminiSuccess || !aiResp.isSuccess) {
            log_e("Gemini Query Failed: %s", aiResp.errorMessage.c_str());
            speakerDriver.playErrorTone();
            uiManager.setCardInfo("Gemini Error", aiResp.errorMessage, 4000);
            animationEngine.setEmotion(MascotEmotion::SAD, 3000);
            uiManager.setSystemStatusText("Ready");
            currentSystemState = SystemState::STANDBY_IDLE;
            continue;
        }

        log_i("AI Response: '%s' [Emotion: %d]", aiResp.speechText.c_str(), (int)aiResp.emotion);

        // 3. Apply Display & Mascot Emotion Commands
        animationEngine.setEmotion(aiResp.emotion);
        if (aiResp.displayCmd.isValid) {
            uiManager.setDisplayMode(aiResp.displayCmd.mode, aiResp.displayCmd.durationMs);
            if (aiResp.displayCmd.title.length() > 0) {
                uiManager.setCardInfo(aiResp.displayCmd.title, aiResp.displayCmd.subtitle, aiResp.displayCmd.durationMs);
            }
        }
        uiManager.setSpeechBubble(aiResp.speechText, 8000);

        // 4. Execute Smart Home Actions
        if (aiResp.homeCmd.isValid) {
            String feedback;
            homeController.executeCommand(aiResp.homeCmd, &feedback);
        }

        // 5. Generate & Play Voice Response (TTS)
        currentSystemState = SystemState::SPEAKING_RESPONSE;
        uiManager.setSystemStatusText("Speaking...");
        
        bool ttsOk = ttsClient.speakText(aiResp.speechText, aiResp.languageCode);
        if (!ttsOk) {
            log_i("Playing cute offline mascot syllable voice for response...");
            speakerDriver.speakMascotVoice(aiResp.speechText);
        }

        // Finished turn
        uiManager.setSystemStatusText("Ready");
        currentSystemState = SystemState::STANDBY_IDLE;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// -------------------------------------------------------------------------
// Background System Monitoring Task (NTP, Smart Home, Weather)
// -------------------------------------------------------------------------
void systemTaskFunc(void* parameter) {
    // Initial non-blocking WiFi connect attempt
    wifiManager.connect(WIFI_SSID, WIFI_PASSWORD);
    if (wifiManager.isConnected()) {
        ntpClient.begin();
        weatherClient.fetchWeather(WEATHER_CITY);
    }

    uint32_t lastWeatherCheck = millis();

    while (true) {
        wifiManager.update();
        ntpClient.update();
        homeController.update();

        // Check weather every 15 minutes
        if (wifiManager.isConnected() && (millis() - lastWeatherCheck > (15 * 60 * 1000))) {
            lastWeatherCheck = millis();
            weatherClient.fetchWeather(WEATHER_CITY);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// -------------------------------------------------------------------------
// Arduino Setup & Bootstrap
// -------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200);
    log_i("=================================================");
    log_i("  Starting Project Bondhu (Version %s)", PROJECT_BONDHU_VERSION);
    log_i("  Mascot: %s (Kitsune Fox), User: %s", MASCOT_NAME, DEFAULT_USER_NAME);
    log_i("=================================================");

    // 1. Initialize Display IMMEDIATELY so pixels turn on at 0ms
    displayDriver.begin();
    animationEngine.begin();
    uiManager.begin();
    uiManager.setSystemStatusText("Bondhu Ready");
    uiManager.setSpeechBubble("Hello! Ami Bondhu.", 5000);

    // 2. Launch Display Task immediately at high priority
    xTaskCreate(displayTaskFunc, "DisplayTask", STACK_SIZE_DISPLAY, NULL, TASK_PRIO_DISPLAY, &hDisplayTask);

    // 3. Allocate Lean Audio Buffer (96KB) in internal RAM
    voiceRecordBuffer = (uint8_t*)malloc(AUDIO_BUF_SIZE);
    if (!voiceRecordBuffer) {
        log_w("Could not allocate 96KB audio buffer, trying 64KB fallback...");
        voiceRecordBuffer = (uint8_t*)malloc(64 * 1024);
    }
    log_i("Audio buffer allocated. Free Heap: %u bytes", (unsigned)ESP.getFreeHeap());

    // 4. Configure Action Button
    pinMode(PIN_BUTTON_ACTION, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_ACTION), onActionButtonPressed, FALLING);

    // 5. Initialize Audio & AI Drivers
    speakerDriver.begin(SPK_SAMPLE_RATE);
    speakerDriver.playStartupSound();
    contextManager.begin();
    geminiClient.begin();
    ttsClient.begin();

    // 6. Launch Voice and Background System Tasks (Non-blocking)
    xTaskCreate(voiceTaskFunc, "VoiceTask", STACK_SIZE_NETWORK, NULL, TASK_PRIO_NETWORK, &hVoiceTask);
    xTaskCreate(systemTaskFunc, "SystemTask", STACK_SIZE_SYSTEM, NULL, TASK_PRIO_SYSTEM, &hSystemTask);

    currentSystemState = SystemState::STANDBY_IDLE;
    log_i("Setup finished! Free Heap: %u bytes", (unsigned)ESP.getFreeHeap());
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
