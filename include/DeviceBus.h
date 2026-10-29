#pragma once
#include <Arduino.h>
#include <Wire.h>

extern uint8_t oledAddress;
extern uint8_t mpuAddress;
bool i2cDetected(uint8_t address);
