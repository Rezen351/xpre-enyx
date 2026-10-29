#pragma once

#include <Arduino.h>

class AudioPlayer {
public:
    void begin(uint8_t channel);
    void startAuto(uint32_t phaseMs);
    void startSegment(uint32_t startMs, uint32_t durationMs);
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
    volatile uint32_t requestedStartMs = 0;
    volatile uint32_t requestedEndMs = 36000;
    volatile uint32_t loopStartMs = 0;
    uint8_t channel = 0;
    uint32_t sampleIndex = 0;
    uint32_t segmentEndSample = 288000;
    uint32_t segmentLoopSample = 0;
    int16_t predictor = 0;
    uint8_t stepIndex = 0;
};
