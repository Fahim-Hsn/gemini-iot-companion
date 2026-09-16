#include "home_controller.h"
#include "audio/speaker_driver.h"
#include "display/ui_manager.h"
#include "display/animation_engine.h"

HomeController homeController;

HomeController::HomeController() 
    : _lightState(false), 
      _fanState(false), 
      _timerActive(false), 
      _timerEndTimeMs(0), 
      _timerLabel("") {}

HomeController::~HomeController() {}

bool HomeController::begin() {
    return true;
}

bool HomeController::executeCommand(const SmartHomeCommand& cmd, String* outFeedback) {
    if (!cmd.isValid) return false;

    log_i("Executing Smart Home Command: action=%s, target=%s, val=%d", 
          cmd.action.c_str(), cmd.target.c_str(), cmd.value);

    if (cmd.action == "light_on") {
        _lightState = true;
        if (outFeedback) *outFeedback = "Light switched ON";
        uiManager.setCardInfo("Smart Home", "Light switched ON", 4000);
        return true;
    } 
    else if (cmd.action == "light_off") {
        _lightState = false;
        if (outFeedback) *outFeedback = "Light switched OFF";
        uiManager.setCardInfo("Smart Home", "Light switched OFF", 4000);
        return true;
    }
    else if (cmd.action == "fan_on") {
        _fanState = true;
        if (outFeedback) *outFeedback = "Fan turned ON";
        uiManager.setCardInfo("Smart Home", "Fan turned ON", 4000);
        return true;
    }
    else if (cmd.action == "fan_off") {
        _fanState = false;
        if (outFeedback) *outFeedback = "Fan turned OFF";
        uiManager.setCardInfo("Smart Home", "Fan turned OFF", 4000);
        return true;
    }
    else if (cmd.action == "set_timer") {
        uint32_t durationSec = (cmd.value > 0) ? (uint32_t)cmd.value : 60;
        _timerActive = true;
        _timerEndTimeMs = millis() + (durationSec * 1000);
        _timerLabel = (cmd.metadata.length() > 0) ? cmd.metadata : "Timer";

        if (outFeedback) *outFeedback = "Timer set for " + String(durationSec) + " seconds";
        uiManager.setCardInfo("Timer Started", String(durationSec) + " sec (" + _timerLabel + ")", 5000);
        return true;
    }
    else if (cmd.action == "set_alarm") {
        if (outFeedback) *outFeedback = "Alarm scheduled for " + cmd.metadata;
        uiManager.setCardInfo("Alarm Set", cmd.metadata, 5000);
        return true;
    }

    return false;
}

uint32_t HomeController::getTimerRemainingSec() const {
    if (!_timerActive) return 0;
    uint32_t now = millis();
    if (now >= _timerEndTimeMs) return 0;
    return (_timerEndTimeMs - now) / 1000;
}

void HomeController::triggerAlarmRing() {
    _timerActive = false;
    log_i("Timer reached zero! Ringing alarm chime...");
    
    uiManager.setCardInfo("TIMER COMPLETE!", _timerLabel, 8000);
    animationEngine.setEmotion(MascotEmotion::EXCITED, 6000);

    for (int i = 0; i < 3; i++) {
        speakerDriver.playSuccessChime();
        delay(200);
    }
}

void HomeController::update() {
    if (_timerActive && millis() >= _timerEndTimeMs) {
        triggerAlarmRing();
    }
}
