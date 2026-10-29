#pragma once
#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <DHT.h>
#include "Config.h"
#include "AudioPlayer.h"
class Hardware {
public:
    void begin();
    void update(uint32_t now);
    void probe();
    void report();
    void beep(uint16_t hz = 1200, uint16_t duration = 70);
    void startAutoAudio(uint32_t phaseMs);
    void stopAudio();
    bool audioActive() const { return autoAudio.active(); }
    void calibrate();
    int takeSteps();
    bool takeClick();
    bool takeHold();
    bool takeTouch();
    bool mpuReady = false, dhtReady = false, sound = true, touchActive = false;
    float temperature = NAN, humidity = NAN;
    float ax = 0, ay = 0, az = 0, gyroX = 0, gyroY = 0, gyroZ = 0;
    float motion = 0;
    float gravityBaseline = 9.80665f;
    int32_t encoderPosition = 0;
    uint32_t clicks = 0;
    uint32_t touchEvents = 0;
private:
    Adafruit_MPU6050 mpu;
    DHT dht{Config::dhtPin, Config::dhtType};
    AudioPlayer autoAudio;
    uint32_t lastDht = 0, lastMpu = 0, buttonChanged = 0, pressedAt = 0, audioUntil = 0, touchChanged = 0;
    uint8_t previousAB = 0;
    int quarterSteps = 0, steps = 0;
    bool rawButton = true, stableButton = true, click = false, hold = false, held = false;
    bool rawTouch = false, stableTouch = false, touchEvent = false;
};
