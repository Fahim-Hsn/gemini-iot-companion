#include "wifi_manager.h"
#include "display/ui_manager.h"

WiFiManager wifiManager;

WiFiManager::WiFiManager() : _lastReconnectAttempt(0), _wasConnected(false) {}

WiFiManager::~WiFiManager() {}

bool WiFiManager::connect(const char* ssid, const char* password) {
    if (WiFi.status() == WL_CONNECTED) return true;

    log_i("Connecting to WiFi: %s ...", ssid);
    uiManager.setSystemStatusText("Connecting WiFi...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        _wasConnected = true;
        log_i("WiFi Connected! IP: %s, RSSI: %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
        uiManager.setWiFiStatus(true, WiFi.RSSI());
        uiManager.setSystemStatusText("Online");
        return true;
    } else {
        log_w("WiFi Connection Failed!");
        uiManager.setWiFiStatus(false, 0);
        uiManager.setSystemStatusText("WiFi Offline");
        return false;
    }
}

bool WiFiManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

int WiFiManager::getRSSI() {
    return WiFi.RSSI();
}

String WiFiManager::getIPAddress() {
    return WiFi.localIP().toString();
}

void WiFiManager::update() {
    bool currentConn = (WiFi.status() == WL_CONNECTED);
    if (currentConn != _wasConnected) {
        _wasConnected = currentConn;
        uiManager.setWiFiStatus(currentConn, currentConn ? WiFi.RSSI() : 0);
        uiManager.setSystemStatusText(currentConn ? "Online" : "WiFi Lost");
    }

    if (!currentConn && (millis() - _lastReconnectAttempt > 10000)) {
        _lastReconnectAttempt = millis();
        log_i("Attempting WiFi auto-reconnect...");
        WiFi.reconnect();
    }
}
