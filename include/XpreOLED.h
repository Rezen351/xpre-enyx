#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

enum class Mood : uint8_t { Auto, Normal, Curious, Happy, Surprised, Love, Sleepy, Wink };

class XpreOLED {
public:
    XpreOLED();
    bool begin();
    void showFullScreen();
    void setMood(Mood value) { mood = value; }
    void text(const char* title, const char* body);
    void testPattern(uint8_t stage);
    void dim(bool value);
    void animate(uint32_t now);
private:
    // SH1106 has 132 internal columns; U8g2 applies the required 2-column offset.
    U8G2_SH1106_128X64_NONAME_F_SW_I2C display;
    Mood mood = Mood::Auto;
    uint32_t lastFrame = 0;
    uint32_t nextBlink = 2000;
    uint32_t blinkStart = 0;
    float gazeX = 0, gazeY = 0;
    float eyeWidth = 29, leftHeight = 34, rightHeight = 34;
    void eye(int x, int y, int w, int h, bool happy, bool sleepy);
};
