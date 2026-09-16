#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();

    bool connect(const char* ssid = WIFI_SSID, const char* password = WIFI_PASSWORD);
    bool isConnected();
    int getRSSI();
    String getIPAddress();
    void update();

private:
    uint32_t _lastReconnectAttempt;
    bool     _wasConnected;
};

extern WiFiManager wifiManager;
