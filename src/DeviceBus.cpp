#include "DeviceBus.h"
#include "Config.h"

uint8_t oledAddress = 0x3C;
uint8_t mpuAddress = 0x68;

bool i2cDetected(uint8_t address) {
    if (digitalRead(Config::sda)==LOW || digitalRead(Config::scl)==LOW) return false;
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}
