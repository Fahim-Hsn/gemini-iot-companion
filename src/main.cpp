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

// Audio buffer in PSRAM / Heap for voice recording
#define AUDIO_BUF_SIZE (MIC_SAMPLE_RATE * sizeof(int16_t) * MIC_RECORD_MAX_SEC)
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
// Display Animation FreeRTOS Task (30 FPS)
// -------------------------------------------------------------------------
void displayTaskFunc(void* parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(33); // ~30 FPS

    log_i("Display Task running on Core %d", xPortGetCoreID());

    while (true) {
        float mouthLevel = speakerDriver.getCurrentMouthLevel();

        // 1. Update mascot physics & internal timers
        animationEngine.update(mouthLevel);

        // 2. Render mascot character on canvas
        animationEngine.render(displayDriver.getCanvas(), mouthLevel);

        // 3. Render active overlays (Clock, weather, speech bubble, status)
        uiManager.renderOverlay(displayDriver.getCanvas());

        // 4. Push frame to physical ST7789 LCD via SPI DMA
        displayDriver.pushCanvas();

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// -------------------------------------------------------------------------
// Voice Interaction Pipeline (Record -> Gemini API -> TTS -> Speaker + Mascot)
// -------------------------------------------------------------------------
void voiceTaskFunc(void* parameter) {
    log_i("Voice Task running on Core %d", xPortGetCoreID());

    while (true) {
        // Wait for voice activation trigger (Button or VAD)
        if (!triggerVoiceChat) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        triggerVoiceChat = false;

        log_i("--- Starting Voice Conversation Turn ---");
        currentSystemState = SystemState::LISTENING_VOICE;
        uiManager.setSystemStatusText("Listening...");
        animationEngine.setEmotion(MascotEmotion::LISTENING);
        speakerDriver.playWakeChime();

        // 1. Record voice from INMP441 Microphone
        size_t recordedBytes = 0;
        bool recordedOk = micDriver.startRecording(voiceRecordBuffer, AUDIO_BUF_SIZE, &recordedBytes, 6000);

        if (!recordedOk || recordedBytes < (MIC_SAMPLE_RATE * sizeof(int16_t) / 2)) {
            log_w("No valid voice audio recorded.");
            speakerDriver.playErrorTone();
            animationEngine.setEmotion(MascotEmotion::CONFUSED, 2000);
            uiManager.setSystemStatusText("Ready");
            currentSystemState = SystemState::STANDBY_IDLE;
            continue;
        }

        // 2. Processing with Gemini 2.0 Flash API
        currentSystemState = SystemState::CALLING_GEMINI;
        uiManager.setSystemStatusText("Thinking...");
        animationEngine.setEmotion(MascotEmotion::THINKING);
        speakerDriver.playReadyBeep();

        AIResponse aiResp;
        bool geminiSuccess = geminiClient.processAudioQuery(voiceRecordBuffer, recordedBytes, &aiResp);

        if (!geminiSuccess || !aiResp.isSuccess) {
            log_e("Gemini Query Failed: %s", aiResp.errorMessage.c_str());
            speakerDriver.playErrorTone();
            uiManager.setCardInfo("Error", "Could not reach Gemini", 3000);
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
            log_w("TTS direct synthesis failed, falling back to chime.");
            speakerDriver.playSuccessChime();
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
    uint32_t lastWeatherCheck = 0;

    while (true) {
        wifiManager.update();
        ntpClient.update();
        homeController.update();

        // Check weather every 15 minutes
        if (wifiManager.isConnected() && (millis() - lastWeatherCheck > (15 * 60 * 1000) || lastWeatherCheck == 0)) {
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
    delay(1000);
    log_i("=================================================");
    log_i("  Starting Project Bondhu (Version %s)", PROJECT_BONDHU_VERSION);
    log_i("  Mascot: %s (Kitsune Fox), User: %s", MASCOT_NAME, DEFAULT_USER_NAME);
    log_i("=================================================");

    // 1. Allocate Audio Buffer in PSRAM or Heap
    voiceRecordBuffer = (uint8_t*)malloc(AUDIO_BUF_SIZE);
    if (!voiceRecordBuffer) {
        log_e("Critical: Failed to allocate voice record buffer!");
    } else {
        log_i("Allocated %u bytes for audio buffer.", AUDIO_BUF_SIZE);
    }

    // 2. Configure Action Button
    pinMode(PIN_BUTTON_ACTION, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_ACTION), onActionButtonPressed, FALLING);

    // 3. Initialize Display & Animation Engine
    displayDriver.begin();
    animationEngine.begin();
    uiManager.begin();
    uiManager.setSystemStatusText("Booting Bondhu...");

    // 4. Initialize Audio Drivers
    speakerDriver.begin(SPK_SAMPLE_RATE);
    micDriver.begin();

    // 5. Initialize AI & Context Layer
    contextManager.begin();
    geminiClient.begin();
    ttsClient.begin();

    // 6. Play startup chime & animation
    speakerDriver.playWakeChime();
    animationEngine.setEmotion(MascotEmotion::WAKING_UP, 2500);

    // 7. Connect WiFi & Sync Time
    if (wifiManager.connect(WIFI_SSID, WIFI_PASSWORD)) {
        ntpClient.begin();
        weatherClient.fetchWeather(WEATHER_CITY);
    }

    // 8. Launch Multi-Core FreeRTOS Tasks
    xTaskCreatePinnedToCore(displayTaskFunc, "DisplayTask", STACK_SIZE_DISPLAY, NULL, TASK_PRIO_DISPLAY, &hDisplayTask, 0);
    xTaskCreatePinnedToCore(voiceTaskFunc, "VoiceTask", STACK_SIZE_NETWORK, NULL, TASK_PRIO_NETWORK, &hVoiceTask, 1);
    xTaskCreatePinnedToCore(systemTaskFunc, "SystemTask", STACK_SIZE_SYSTEM, NULL, TASK_PRIO_SYSTEM, &hSystemTask, 0);

    currentSystemState = SystemState::STANDBY_IDLE;
    uiManager.setSystemStatusText("Ready");
    uiManager.setSpeechBubble("Hello! Ami Bondhu. Kemon achen?", 5000);
    log_i("Project Bondhu initialization completed successfully!");
}

void loop() {
    // Loop idle — tasks handled by FreeRTOS scheduler
    vTaskDelay(pdMS_TO_TICKS(1000));
}
