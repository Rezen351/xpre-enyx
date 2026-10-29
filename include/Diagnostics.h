#pragma once
#include "Hardware.h"
#include "XpreOLED.h"
class Diagnostics {
public:
    Diagnostics(Hardware& hw, XpreOLED& screen, bool& ready) : hw(hw), screen(screen), ready(ready) {}
    void start(const char* target);
    void update(uint32_t now);
    void stop();
    bool active() const { return running; }
private:
    Hardware& hw;
    XpreOLED& screen;
    bool& ready;
    bool running=false, all=false;
    uint8_t stage=0, address=1;
    uint32_t stageAt=0;
    int32_t startPosition=0;
    uint32_t startClicks=0;
    char target[12]{};
};
