#include "XpreOLED.h"
#include "DeviceBus.h"
#include "Config.h"
#include <math.h>

XpreOLED::XpreOLED() : display(128, 64, &Wire, -1) {}

bool XpreOLED::begin() {
    if (initialized) return true;
    if (!i2cDetected(oledAddress)) return false;
    Wire.setClock(Config::i2cClock);
    Wire.setTimeOut(Config::i2cTimeout);
    if (!display.begin(oledAddress, true)) return false;
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.display();
    initialized = true;
    return true;
}

void XpreOLED::showFullScreen() {
    display.fillScreen(SH110X_WHITE);
    display.display();
}

void XpreOLED::eye(int x, int y, int w, int h, bool happy, bool sleepy) {
    h = max(2, h);
    int radius = min(8, min(w, h) / 2);
    display.fillRoundRect(x - w / 2, y - h / 2, w, h, radius, SH110X_WHITE);
    if (happy && h > 10) {
        display.fillRoundRect(x - w / 2 - 2, y - h / 2 + h / 3, w + 4, h, 9, SH110X_BLACK);
    }
    if (sleepy && h > 8) {
        display.fillRect(x - w / 2, y - h / 2, w, h / 3, SH110X_BLACK);
    }
}

void XpreOLED::animate(uint32_t now) {
    if (now - lastFrame < 33) return;
    lastFrame = now;
    uint32_t phase = now % 36000;
    switch (mood) {
        case Mood::Normal: phase = 1000; break;
        case Mood::Curious: phase = 8000; break;
        case Mood::Happy: phase = 13000; break;
        case Mood::Surprised: phase = 18000; break;
        case Mood::Love: phase = 22000; break;
        case Mood::Sleepy: phase = 27000; break;
        case Mood::Wink: phase = 33400; break;
        default: break;
    }

    float tx = 0, ty = 0, tw = 29, lh = 34, rh = 34;
    bool happy = false, sleepy = false, hearts = false;
    if (phase < 6000) {
        tx = phase < 2000 ? 0 : (phase < 4000 ? -10 : 10);
    } else if (phase < 11000) {
        tx = -12; ty = -3; lh = 25; rh = 39; tw = 31;
    } else if (phase < 16000) {
        happy = true; ty = -2 - fabsf(sinf(now * 0.008f)) * 4;
    } else if (phase < 20000) {
        lh = rh = 42; tw = 35; ty = -2;
    } else if (phase < 25000) {
        happy = true; hearts = true; lh = rh = 33; tw = 30;
    } else if (phase < 31000) {
        sleepy = true; lh = rh = 15; ty = 4; tw = 32;
    } else {
        lh = rh = 35;
        if (phase > 33000 && phase < 33700) lh = 3;
        tx = 5;
    }

    gazeX += (tx - gazeX) * 0.16f;
    gazeY += (ty - gazeY) * 0.16f;
    eyeWidth += (tw - eyeWidth) * 0.18f;
    leftHeight += (lh - leftHeight) * 0.20f;
    rightHeight += (rh - rightHeight) * 0.20f;

    if (!blinkStart && int32_t(now - nextBlink) >= 0) blinkStart = now;
    float openness = 1;
    if (blinkStart) {
        uint32_t elapsed = now - blinkStart;
        if (elapsed < 90) openness = 1 - elapsed / 90.0f;
        else if (elapsed < 140) openness = 0;
        else if (elapsed < 280) openness = (elapsed - 140) / 140.0f;
        else {
            blinkStart = 0;
            nextBlink = now + random(2200, 5200);
        }
    }

    float breath = sinf(now * 0.0025f) * 1.2f;
    int x = 64 + int(gazeX);
    int y = 32 + int(gazeY + breath);
    display.clearDisplay();
    eye(x - 25, y, int(eyeWidth), int(leftHeight * openness), happy, sleepy);
    eye(x + 25, y, int(eyeWidth), int(rightHeight * openness), happy, sleepy);

    if (hearts) {
        int hy = 16 - int((now % 1500) / 180);
        for (int hx : {9, 112}) {
            display.fillCircle(hx - 2, hy, 2, SH110X_WHITE);
            display.fillCircle(hx + 2, hy, 2, SH110X_WHITE);
            display.fillTriangle(hx - 4, hy, hx + 4, hy, hx, hy + 6, SH110X_WHITE);
        }
    }
    if (sleepy) {
        display.setTextSize(1);
        display.setCursor(109, 14 + int(breath));
        display.print("z");
        display.setCursor(119, 10);
        display.print("Z");
    }
    display.display();
}

void XpreOLED::text(const char* title, const char* body) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setTextWrap(true);
    display.setCursor(0, 9);
    display.println(title);
    display.drawFastHLine(0, 12, 128, SH110X_WHITE);

    int row = 0;
    const char* cursor = body;
    while (*cursor && row < 5) {
        char line[64]{};
        size_t length = 0;
        while (cursor[length] && cursor[length] != '\n' && length < sizeof(line) - 1) ++length;
        memcpy(line, cursor, length);
        line[length] = 0;
        display.setCursor(0, 23 + row * 9);
        display.print(line);
        ++row;
        cursor += length;
        if (*cursor == '\n') ++cursor;
    }
    display.display();
}

void XpreOLED::testPattern(uint8_t stage) {
    display.clearDisplay();
    if (stage == 0) {
        display.fillScreen(SH110X_WHITE);
    } else if (stage == 2) {
        for (int x = 0; x < 128; x += 8) display.drawFastVLine(x, 0, 64, SH110X_WHITE);
        for (int y = 0; y < 64; y += 8) display.drawFastHLine(0, y, 128, SH110X_WHITE);
        display.drawRect(0, 0, 128, 64, SH110X_WHITE);
    } else if (stage == 3) {
        for (int y = 0; y < 64; y += 8) {
            for (int x = 0; x < 128; x += 8) {
                if (((x + y) / 8) % 2 == 0) display.fillRect(x, y, 8, 8, SH110X_WHITE);
            }
        }
    }
    display.display();
}

void XpreOLED::dim(bool value) {
    display.setContrast(value ? 0x10 : 0xCF);
}
