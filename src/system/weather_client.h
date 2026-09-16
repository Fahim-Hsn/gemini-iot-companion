#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"

class WeatherClient {
public:
    WeatherClient();
    ~WeatherClient();

    bool begin();
    bool fetchWeather(const String& city = WEATHER_CITY);
    
    int getTemperature() const { return _temperatureC; }
    String getCondition() const { return _condition; }
    String getCity() const { return _city; }

private:
    int      _temperatureC;
    String   _condition;
    String   _city;
    uint32_t _lastFetchTime;
};

extern WeatherClient weatherClient;
