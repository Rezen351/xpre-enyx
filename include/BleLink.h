#pragma once
#include <Arduino.h>
namespace BleLink {
void begin();
void update();
bool takeMessage(char* output, size_t capacity);
bool connected();
void publish(const char* text);
}
