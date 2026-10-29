#include "Hardware.h"
#include "DeviceBus.h"
#include <math.h>

void Hardware::begin() {
    pinMode(Config::encoderA, INPUT_PULLUP);
    pinMode(Config::encoderB, INPUT_PULLUP);
    pinMode(Config::button, INPUT_PULLUP);
    pinMode(Config::touchPin, INPUT_PULLDOWN);
    previousAB = (digitalRead(Config::encoderA) << 1) | digitalRead(Config::encoderB);
    rawTouch = stableTouch = Config::touchActiveHigh ? digitalRead(Config::touchPin) : !digitalRead(Config::touchPin);
    dht.begin();
    ledcSetup(0, 2000, 8);
    ledcAttachPin(Config::audioPin, 0);
    ledcWrite(0, 0);
    probe();
}
void Hardware::probe() {
    mpuReady = false;
    for (uint8_t address : {uint8_t(0x68), uint8_t(0x69)}) {
        if (i2cDetected(address) && mpu.begin(address, &Wire)) {
            mpuAddress = address;
            mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
            mpu.setGyroRange(MPU6050_RANGE_500_DEG);
            mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
            mpuReady = true;
            break;
        }
    }
}
void Hardware::update(uint32_t now) {
    static const int8_t transitions[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
    uint8_t ab = (digitalRead(Config::encoderA) << 1) | digitalRead(Config::encoderB);
    if (ab != previousAB) {
        quarterSteps += transitions[(previousAB << 2) | ab];
        previousAB = ab;
        if (abs(quarterSteps) >= 4) {
            int direction = quarterSteps > 0 ? 1 : -1;
            steps += direction;
            encoderPosition += direction;
            quarterSteps = 0;
        }
    }
    bool raw = digitalRead(Config::button);
    if (raw != rawButton) { rawButton = raw; buttonChanged = now; }
    if (now - buttonChanged >= 30 && stableButton != rawButton) {
        stableButton = rawButton;
        if (!stableButton) { pressedAt = now; held = false; }
        else if (!held) { click = true; ++clicks; }
    }
    if (!stableButton && !held && now - pressedAt >= 800) {
        hold = true; held = true;
    }
    bool rawTouchState = Config::touchActiveHigh ? digitalRead(Config::touchPin) : !digitalRead(Config::touchPin);
    if (rawTouchState != rawTouch) { rawTouch = rawTouchState; touchChanged = now; }
    if (now - touchChanged >= 30 && stableTouch != rawTouch) {
        stableTouch = rawTouch;
        touchActive = stableTouch;
        if (stableTouch) { touchEvent = true; ++touchEvents; }
    }
    if (now - lastDht >= Config::sensorInterval) {
        lastDht = now;
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();
        dhtReady = isfinite(temperature) && isfinite(humidity);
    }
    if (mpuReady && now - lastMpu >= 100) {
        lastMpu = now;
        if (!i2cDetected(mpuAddress)) { mpuReady = false; motion = 0; return; }
        sensors_event_t a{}, g{}, t{};
        if (!mpu.getEvent(&a, &g, &t)) { mpuReady = false; return; }
        ax=a.acceleration.x; ay=a.acceleration.y; az=a.acceleration.z;
        gyroX=g.gyro.x; gyroY=g.gyro.y; gyroZ=g.gyro.z;
        // Remove gravity magnitude so a stationary board is not always excited.
        float movement = fabsf(sqrtf(ax*ax + ay*ay + az*az) - gravityBaseline);
        motion = motion * 0.7f + movement * 0.3f;
    }
    // Audio deadlines are serviced without blocking input or animation.
    if (audioUntil && int32_t(now - audioUntil) >= 0) {
        ledcWriteTone(0, 0); audioUntil = 0;
    }
}
void Hardware::beep(uint16_t hz, uint16_t duration) {
    if (!sound) return;
    ledcWriteTone(0, hz);
    audioUntil = millis() + duration;
}
void Hardware::calibrate() {
    if (!mpuReady) { Serial.println("MPU unavailable; calibration skipped."); return; }
    gravityBaseline = sqrtf(ax*ax+ay*ay+az*az);
    if (gravityBaseline < 7 || gravityBaseline > 12) gravityBaseline = 9.80665f;
    motion = 0;
    Serial.printf("Gravity baseline %.3f m/s2; keep board still when calibrating.\n", gravityBaseline);
}
int Hardware::takeSteps() { int value=steps; steps=0; return value; }
bool Hardware::takeClick() { bool value=click; click=false; return value; }
bool Hardware::takeHold() { bool value=hold; hold=false; return value; }
bool Hardware::takeTouch() { bool value=touchEvent; touchEvent=false; return value; }
void Hardware::report() {
    Serial.printf("MPU6050: %s address=0x%02X motion=%.2f m/s2\n", mpuReady?"READY":"UNAVAILABLE", mpuAddress, motion);
    if (mpuReady) Serial.printf("  accel=%.2f,%.2f,%.2f m/s2 gyro=%.2f,%.2f,%.2f rad/s\n",ax,ay,az,gyroX,gyroY,gyroZ);
    if (dhtReady) Serial.printf("DHT%u: %.1f C / %.1f %%RH\n",Config::dhtType,temperature,humidity);
    else Serial.printf("DHT%u: NO VALID READING (GPIO%u)\n",Config::dhtType,Config::dhtPin);
    Serial.printf("Encoder: position=%ld clicks=%lu button=%s (manual verification required)\n",long(encoderPosition),static_cast<unsigned long>(clicks),digitalRead(Config::button)?"UP":"DOWN");
    Serial.printf("Touch GPIO%u: %s events=%lu\n",Config::touchPin,touchActive?"TOUCHED":"released",static_cast<unsigned long>(touchEvents));
    Serial.printf("Audio GPIO%u: %s (sound must be checked by listening)\n",Config::audioPin,sound?"ON":"MUTED");
}
