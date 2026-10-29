#pragma once
#include <Arduino.h>
namespace Config {
constexpr uint8_t sda = 8, scl = 7;
constexpr uint8_t encoderA = 2, encoderB = 3, button = 4;
constexpr uint8_t dhtPin = 5, audioPin = 6;
constexpr uint8_t dhtType = 11; // 11 = DHT11, 22 = DHT22
constexpr uint32_t i2cClock = 400000, i2cTimeout = 25;
constexpr uint32_t sensorInterval = 2500, retryInterval = 10000;
constexpr uint32_t idleSleep = 120000;
constexpr size_t commandSize = 128;
constexpr char bleName[] = "Xpre-Enyx";
constexpr char serviceUuid[] = "a6e40001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char rxUuid[] = "a6e40002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char txUuid[] = "a6e40003-b5a3-f393-e0a9-e50e24dcca9e";
}
