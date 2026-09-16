#include "ntp_client.h"
#include "display/ui_manager.h"

NTPClient ntpClient;

NTPClient::NTPClient() : _isSynced(false), _lastSyncCheck(0) {}

NTPClient::~NTPClient() {}

bool NTPClient::begin(long gmtOffsetSec, int daylightOffsetSec) {
    configTime(gmtOffsetSec, daylightOffsetSec, NTP_SERVER_1, NTP_SERVER_2);
    _lastSyncCheck = millis();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
        _isSynced = true;
        log_i("NTP Time Synchronized: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        uiManager.setTime(getFormattedTime(), getFormattedDate());
        return true;
    }
    return false;
}

String NTPClient::getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "12:00 PM";
    }

    char buf[16];
    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
    snprintf(buf, sizeof(buf), "%02d:%02d %s", hour12, timeinfo.tm_min, ampm);
    return String(buf);
}

String NTPClient::getFormattedDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Wed, Sep 16";
    }

    static const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    char buf[32];
    snprintf(buf, sizeof(buf), "%s, %s %d", days[timeinfo.tm_wday], months[timeinfo.tm_mon], timeinfo.tm_mday);
    return String(buf);
}

int NTPClient::getHour() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) return timeinfo.tm_hour;
    return 12;
}

int NTPClient::getMinute() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) return timeinfo.tm_min;
    return 0;
}

void NTPClient::update() {
    if (millis() - _lastSyncCheck > 1000) {
        _lastSyncCheck = millis();
        uiManager.setTime(getFormattedTime(), getFormattedDate());
    }
}
