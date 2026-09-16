#include "weather_client.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "display/ui_manager.h"
#include "display/animation_engine.h"

WeatherClient weatherClient;

WeatherClient::WeatherClient()
    : _temperatureC(28),
      _condition("Sunny"),
      _city(WEATHER_CITY),
      _lastFetchTime(0) {}

WeatherClient::~WeatherClient() {}

bool WeatherClient::begin() {
    return true;
}

bool WeatherClient::fetchWeather(const String& city) {
    if (String(OPENWEATHER_API_KEY) == "YOUR_OPENWEATHER_KEY" || String(OPENWEATHER_API_KEY).length() == 0) {
        log_i("Weather API Key not configured. Using default weather data.");
        uiManager.setWeather(_temperatureC, _condition, _city);
        return true;
    }

    WiFiClient client;
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + String(WEATHER_COUNTRY_CODE) + "&units=metric&appid=" + String(OPENWEATHER_API_KEY);

    http.begin(client, url);
    http.setTimeout(8000);

    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        http.end();

        JsonDocument doc;
        deserializeJson(doc, payload);

        _temperatureC = (int)round(doc["main"]["temp"].as<float>());
        _condition = doc["weather"][0]["main"].as<String>();
        _city = doc["name"].as<String>();
        _lastFetchTime = millis();

        uiManager.setWeather(_temperatureC, _condition, _city);
        log_i("Live Weather fetched: %s, %d°C, %s", _city.c_str(), _temperatureC, _condition.c_str());

        // Update mascot emotion if extreme weather
        if (_condition.indexOf("Rain") >= 0 || _condition.indexOf("Storm") >= 0) {
            animationEngine.setEmotion(MascotEmotion::SAD, 3000);
        }

        return true;
    } else {
        log_w("Weather fetch failed (HTTP %d)", httpCode);
        http.end();
        return false;
    }
}
