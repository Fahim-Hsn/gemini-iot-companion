#pragma once

#include <Arduino.h>
#include <time.h>
#include "config.h"

class NTPClient {
public:
    NTPClient();
    ~NTPClient();

    bool begin(long gmtOffsetSec = TIMEZONE_OFFSET_SEC, int daylightOffsetSec = 0);
    
    // Returns true if NTP time has been synced
    bool isSynced() const { return _isSynced; }

    String getFormattedTime(); // e.g. "10:45 AM"
    String getFormattedDate(); // e.g. "Wed, Sep 16"
    int getHour();
    int getMinute();

    void update(); // Sync check & timer check

private:
    bool     _isSynced;
    uint32_t _lastSyncCheck;
};

extern NTPClient ntpClient;
