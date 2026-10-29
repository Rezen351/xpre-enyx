#include "AudioPlayer.h"
#include "AutoAudioAsset.h"

namespace {
const int16_t stepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
    143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
    4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086,
    29794, 32767
};
const int8_t indexTable[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
int16_t clamp16(int32_t value) {
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return static_cast<int16_t>(value);
}
}

void AudioPlayer::begin(uint8_t outputChannel) {
    channel = outputChannel;
    if (!task) xTaskCreate(taskEntry, "xpre_audio", 4096, this, 1, &task);
}
void AudioPlayer::startAuto(uint32_t phaseMs) {
    requestedStartMs = phaseMs % 36000UL;
    requestedEndMs = 36000UL;
    loopStartMs = 0;
    decoderReset = true;
    playing = true;
    ledcSetup(channel, 31250, 8);
    ledcWrite(channel, 128);
}
void AudioPlayer::startSegment(uint32_t startMs, uint32_t durationMs) {
    const uint32_t start = min<uint32_t>(startMs, 35999UL);
    const uint32_t end = min<uint32_t>(start + max<uint32_t>(durationMs, 1UL), 36000UL);
    requestedStartMs = start;
    requestedEndMs = end;
    loopStartMs = start;
    decoderReset = true;
    playing = true;
    ledcSetup(channel, 31250, 8);
    ledcWrite(channel, 128);
}
void AudioPlayer::stop() {
    playing = false;
    decoderReset = true;
    ledcWrite(channel, 0);
}
void AudioPlayer::taskEntry(void* context) { static_cast<AudioPlayer*>(context)->run(); }
void AudioPlayer::resetDecoder(uint32_t phaseMs) {
    predictor = AutoAudio::initialPredictor;
    stepIndex = AutoAudio::initialIndex;
    sampleIndex = 0;
    const uint32_t skip = (phaseMs * AutoAudio::sampleRate) / 1000UL;
    for (uint32_t i = 0; i < skip; ++i) nextSample();
}
int16_t AudioPlayer::nextSample() {
    if (sampleIndex == 0) { ++sampleIndex; return predictor; }
    if (sampleIndex >= AutoAudio::sampleCount) resetDecoder(0);
    const uint32_t encodedIndex = sampleIndex - 1;
    const uint8_t packed = pgm_read_byte(&AutoAudio::data[encodedIndex >> 1]);
    const uint8_t nibble = (encodedIndex & 1U) ? (packed >> 4) : (packed & 0x0FU);
    const int step = stepTable[stepIndex];
    int diff = step >> 3;
    if (nibble & 1) diff += step >> 2;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 4) diff += step;
    predictor = clamp16(predictor + ((nibble & 8) ? -diff : diff));
    int nextIndex = static_cast<int>(stepIndex) + indexTable[nibble];
    if (nextIndex < 0) nextIndex = 0;
    if (nextIndex > 88) nextIndex = 88;
    stepIndex = static_cast<uint8_t>(nextIndex);
    ++sampleIndex;
    return predictor;
}
void AudioPlayer::run() {
    for (;;) {
        if (!playing) { vTaskDelay(1); continue; }
        if (decoderReset) {
            const uint32_t phase = requestedStartMs;
            const uint32_t end = requestedEndMs;
            const uint32_t loop = loopStartMs;
            decoderReset = false;
            segmentEndSample = (end * AutoAudio::sampleRate) / 1000UL;
            segmentLoopSample = (loop * AutoAudio::sampleRate) / 1000UL;
            resetDecoder(phase);
        }
        if (sampleIndex >= segmentEndSample) {
            resetDecoder((segmentLoopSample * 1000UL) / AutoAudio::sampleRate);
        }
        const int16_t sample = nextSample();
        ledcWrite(channel, static_cast<uint8_t>((static_cast<int32_t>(sample) + 32768) >> 8));
        delayMicroseconds(125);
        if ((sampleIndex & 31U) == 0) taskYIELD();
    }
}
