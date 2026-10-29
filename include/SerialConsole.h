#pragma once
#include <Arduino.h>
#include "Config.h"
class SerialConsole {
public:
    void update(void (*handler)(const char*));
private:
    char buffer[Config::commandSize]{};
    size_t used=0;
    bool overflow=false;
};
