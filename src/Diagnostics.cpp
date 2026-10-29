#include "Diagnostics.h"
#include "DeviceBus.h"
#include "Config.h"
void Diagnostics::stop() {
    if (running) Serial.println("TEST stopped; normal operation resumed.");
    running=false;
}
void Diagnostics::start(const char* requested) {
    if (strcmp(requested,"stop")==0) { stop(); return; }
    if (strcmp(requested,"all") && strcmp(requested,"oled") && strcmp(requested,"audio") &&
        strcmp(requested,"encoder") && strcmp(requested,"touch") && strcmp(requested,"i2c") && strcmp(requested,"mpu") && strcmp(requested,"dht")) {
        Serial.println("ERR test target: all|oled|mpu|dht|audio|encoder|touch|i2c|stop"); return;
    }
    strlcpy(target,requested,sizeof(target));
    running=true; all=strcmp(target,"all")==0; stage=0; address=1; stageAt=millis();
    startPosition=hw.encoderPosition; startClicks=hw.clicks;
    startTouchEvents=hw.touchEvents;
    Serial.printf("TEST START %s | SDA=%u SCL=%u | levels SDA=%d SCL=%d\n",target,Config::sda,Config::scl,digitalRead(Config::sda),digitalRead(Config::scl));
    if (strcmp(target,"i2c")==0) { Serial.println("Scanning one address per loop..."); return; }
    // Explicit tests retry devices immediately, independent of background recovery.
    if (all || strcmp(target,"mpu")==0) hw.probe();
    if (all || strcmp(target,"oled")==0) {
        if (!i2cDetected(oledAddress) && i2cDetected(0x3D)) oledAddress=0x3D;
        ready=screen.begin();
        Serial.printf("OLED communication/init: %s. Panel must be checked visually.\n",ready?"ACK / INITIALIZED":"FAIL / UNAVAILABLE");
    }
    hw.report();
    if (all || strcmp(target,"oled")==0) {
        if (ready) screen.testPattern(0);
        Serial.println("OLED pattern: FULL ON -> OFF -> GRID -> CHECKER (1 second each).");
    }
    if (strcmp(target,"audio")==0) { hw.beep(880,200); Serial.println("Audio 880/1320 Hz; verify by listening. Muted setting is respected."); }
    if (strcmp(target,"encoder")==0) Serial.println("Rotate encoder and press/release button during the next 10 seconds.");
    if (strcmp(target,"touch")==0) Serial.printf("Touch test on GPIO%u; touch the sensor during the next 10 seconds.\n", Config::touchPin);
}
void Diagnostics::update(uint32_t now) {
    if (!running) return;
    uint32_t elapsed=now-stageAt;
    if (strcmp(target,"i2c")==0) {
        if (digitalRead(Config::sda)==LOW || digitalRead(Config::scl)==LOW) {
            Serial.println("FAIL: I2C line held LOW; check wiring/module/pull-ups."); stop(); return;
        }
        if (i2cDetected(address)) Serial.printf("I2C ACK 0x%02X (device identity unverified)\n",address);
        if (++address>=127) { Serial.println("TEST END i2c"); running=false; }
        return;
    }
    if (all || strcmp(target,"oled")==0) {
        if (stage<4 && elapsed>=1000) {
            ++stage; stageAt=now;
            if (stage<4 && ready) screen.testPattern(stage);
            if (stage==4) {
                if (!all) { Serial.println("TEST END oled: confirm all four patterns visually."); running=false; return; }
                hw.beep(880,180);
                Serial.println("Audio test: listen for two tones. Rotate encoder and click within 10 seconds.");
                startPosition=hw.encoderPosition; startClicks=hw.clicks;
            }
        } else if (all && stage==4 && elapsed>=400) { hw.beep(1320,180); ++stage; }
        if (all && stage>=4 && elapsed>=10000) {
            Serial.printf("Encoder delta=%ld clicks=%lu (zero means unverified, not proof of damage).\n",long(hw.encoderPosition-startPosition),static_cast<unsigned long>(hw.clicks-startClicks));
            hw.report(); Serial.println("TEST END all: ACK/readings do not verify OLED pixels or audible output."); running=false;
        }
    } else if (strcmp(target,"encoder")==0) {
        if (elapsed>=10000) {
            Serial.printf("TEST END encoder: delta=%ld clicks=%lu\n",long(hw.encoderPosition-startPosition),static_cast<unsigned long>(hw.clicks-startClicks)); running=false;
        }
    } else if (strcmp(target,"touch")==0) {
        if (elapsed>=10000) {
            Serial.printf("TEST END touch: events=%lu final=%s\n",static_cast<unsigned long>(hw.touchEvents-startTouchEvents),hw.touchActive?"TOUCHED":"released"); running=false;
        }
    } else if (strcmp(target,"audio")==0) {
        if (stage==0 && elapsed>=400) { hw.beep(1320,200); stage=1; }
        if (elapsed>=1000) { Serial.println("TEST END audio: listening confirmation required."); running=false; }
    } else if (elapsed>=3000) { hw.report(); Serial.println("TEST END sensor"); running=false; }
}
