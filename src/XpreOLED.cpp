#include "XpreOLED.h"
#include "DeviceBus.h"
#include "Config.h"
#include <math.h>

XpreOLED::XpreOLED() : display(128, 64, &Wire, -1, Config::i2cClock, Config::i2cClock) {}

void XpreOLED::showFullScreen() {
    display.fillScreen(SSD1306_WHITE);
    display.display();
}

bool XpreOLED::begin() {
    if (!i2cDetected(oledAddress)) return false;
    if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddress, true, false)) return false;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
    return true;
}

void XpreOLED::eye(int x, int y, int w, int h, bool happy, bool sleepy) {
    h = max(2, h);
    int radius = min(8, min(w, h) / 2);
    display.fillRoundRect(x - w/2, y - h/2, w, h, radius, SSD1306_WHITE);
    if (happy && h > 10) {
        // A rising lower lid turns the eye into a smiling crescent.
        display.fillRoundRect(x - w/2 - 2, y - h/2 + h/3, w + 4, h, 9, SSD1306_BLACK);
    }
    if (sleepy && h > 8) {
        display.fillRect(x - w/2, y - h/2, w, h/3, SSD1306_BLACK);
    }
}

void XpreOLED::animate(uint32_t now) {
    if (now - lastFrame < 33) return; // About 30 frames per second.
    lastFrame = now;
    uint32_t phase = now % 36000;
    switch (mood) {
        case Mood::Normal: phase=1000; break;
        case Mood::Curious: phase=8000; break;
        case Mood::Happy: phase=13000; break;
        case Mood::Surprised: phase=18000; break;
        case Mood::Love: phase=22000; break;
        case Mood::Sleepy: phase=27000; break;
        case Mood::Wink: phase=33400; break;
        default: break;
    }
    float tx = 0, ty = 0, tw = 29, lh = 34, rh = 34;
    bool happy = false, sleepy = false, hearts = false;
    if (phase < 6000) { // Attentive, little glances.
        tx = phase < 2000 ? 0 : (phase < 4000 ? -10 : 10);
    } else if (phase < 11000) { // Curious head tilt and asymmetric eyes.
        tx = -12; ty = -3; lh = 25; rh = 39; tw = 31;
    } else if (phase < 16000) { // Delighted bounce.
        happy = true; lh = rh = 34;
        ty = -2 - fabsf(sinf(now * 0.008f)) * 4;
    } else if (phase < 20000) { // Surprised, wide eyes.
        lh = rh = 42; tw = 35; ty = -2;
    } else if (phase < 25000) { // Affectionate, with floating pixel hearts.
        happy = true; hearts = true; lh = rh = 33; tw = 30;
    } else if (phase < 31000) { // Drowsy breathing.
        sleepy = true; lh = rh = 15; ty = 4; tw = 32;
    } else { // Wake up, then a playful wink.
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
            display.fillCircle(hx - 2, hy, 2, SSD1306_WHITE);
            display.fillCircle(hx + 2, hy, 2, SSD1306_WHITE);
            display.fillTriangle(hx - 4, hy, hx + 4, hy, hx, hy + 6, SSD1306_WHITE);
        }
    }
    if (sleepy) {
        display.setTextSize(1);
        display.setCursor(109, 5 + int(breath));
        display.print("z");
        display.setCursor(119, 0);
        display.print("Z");
    }
    display.display();
}

void XpreOLED::text(const char* title, const char* body) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(true);
    display.setCursor(0,0);
    display.println(title);
    display.drawFastHLine(0,10,128,SSD1306_WHITE);
    display.setCursor(0,15);
    display.print(body);
    display.display();
}
void XpreOLED::testPattern(uint8_t stage) {
    display.clearDisplay();
    if (stage==0) display.fillScreen(SSD1306_WHITE);
    else if (stage==2) {
        for (int x=0;x<128;x+=8) display.drawFastVLine(x,0,64,SSD1306_WHITE);
        for (int y=0;y<64;y+=8) display.drawFastHLine(0,y,128,SSD1306_WHITE);
        display.drawRect(0,0,128,64,SSD1306_WHITE);
    } else if (stage==3) {
        for (int y=0;y<64;y+=8) for (int x=0;x<128;x+=8)
            if (((x+y)/8)%2==0) display.fillRect(x,y,8,8,SSD1306_WHITE);
    }
    display.display();
}
void XpreOLED::dim(bool value) { display.dim(value); }
