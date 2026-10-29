#pragma once

#include <Arduino.h>

class AudioPlayer {
public:
    void begin(uint8_t channel);
    void startAuto(uint32_t phaseMs);
    void stop();
    bool active() const { return playing; }

private:
    static void taskEntry(void* context);
    void run();
    void resetDecoder(uint32_t phaseMs);
    int16_t nextSample();

    TaskHandle_t task = nullptr;
    volatile bool playing = false;
    volatile bool decoderReset = false;
    volatile uint32_t requestedPhaseMs = 0;
    uint8_t channel = 0;
    uint32_t sampleIndex = 0;
    int16_t predictor = 0;
    uint8_t stepIndex = 0;
};
