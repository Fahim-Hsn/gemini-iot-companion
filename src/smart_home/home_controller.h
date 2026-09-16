#pragma once

#include <Arduino.h>
#include "types.h"

class HomeController {
public:
    HomeController();
    ~HomeController();

    bool begin();
    
    // Execute smart home command
    bool executeCommand(const SmartHomeCommand& cmd, String* outFeedback);

    // Alarm & Timer status
    bool isTimerActive() const { return _timerActive; }
    uint32_t getTimerRemainingSec() const;

    void update(); // Called in background loop

private:
    bool     _lightState;
    bool     _fanState;
    bool     _timerActive;
    uint32_t _timerEndTimeMs;
    String   _timerLabel;

    void triggerAlarmRing();
};

extern HomeController homeController;
