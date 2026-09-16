#include "context_manager.h"
#include <Preferences.h>

ContextManager contextManager;
static Preferences prefs;

ContextManager::ContextManager() : _turnCount(0) {}

ContextManager::~ContextManager() {}

bool ContextManager::begin() {
    return true;
}

void ContextManager::addTurn(const String& role, const String& text) {
    if (_turnCount < MAX_TURNS) {
        _history[_turnCount].role = role;
        _history[_turnCount].text = text;
        _turnCount++;
    } else {
        // Shift history window
        for (size_t i = 1; i < MAX_TURNS; i++) {
            _history[i - 1] = _history[i];
        }
        _history[MAX_TURNS - 1].role = role;
        _history[MAX_TURNS - 1].text = text;
    }
}

void ContextManager::clearHistory() {
    _turnCount = 0;
}

void ContextManager::loadProfile(PersonalityProfile* outProfile) {
    if (!outProfile) return;

    prefs.begin("bondhu_cfg", true);
    outProfile->userName = prefs.getString("user_name", DEFAULT_USER_NAME);
    outProfile->mascotName = prefs.getString("mascot_name", MASCOT_NAME);
    outProfile->playfulness = prefs.getFloat("playful", 0.85f);
    outProfile->proactiveGreeting = prefs.getBool("greet_en", true);
    outProfile->morningGreetingHour = prefs.getInt("m_greet_hr", 8);
    outProfile->eveningGreetingHour = prefs.getInt("e_greet_hr", 22);
    outProfile->speechVolume = prefs.getInt("volume", 85);
    prefs.end();

    log_i("Loaded Profile for User: %s, Mascot: %s", outProfile->userName.c_str(), outProfile->mascotName.c_str());
}

void ContextManager::saveProfile(const PersonalityProfile& profile) {
    prefs.begin("bondhu_cfg", false);
    prefs.putString("user_name", profile.userName);
    prefs.putString("mascot_name", profile.mascotName);
    prefs.putFloat("playful", profile.playfulness);
    prefs.putBool("greet_en", profile.proactiveGreeting);
    prefs.putInt("m_greet_hr", profile.morningGreetingHour);
    prefs.putInt("e_greet_hr", profile.eveningGreetingHour);
    prefs.putInt("volume", profile.speechVolume);
    prefs.end();

    log_i("Saved updated profile to NVS.");
}
