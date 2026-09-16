#pragma once

#include <Arduino.h>
#include "types.h"

struct ChatTurn {
    String role;
    String text;
};

class ContextManager {
public:
    ContextManager();
    ~ContextManager();

    bool begin();
    
    // Add turn to conversation history
    void addTurn(const String& role, const String& text);
    void clearHistory();

    // Persona settings persist in NVS
    void loadProfile(PersonalityProfile* outProfile);
    void saveProfile(const PersonalityProfile& profile);

private:
    static const size_t MAX_TURNS = 8;
    ChatTurn _history[MAX_TURNS];
    size_t   _turnCount;
};

extern ContextManager contextManager;
