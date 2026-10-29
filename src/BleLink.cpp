#include "BleLink.h"
#include "Config.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
namespace {
struct Message { char text[97]; };
QueueHandle_t inbox = nullptr;
BLECharacteristic* tx = nullptr;
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
bool linked = false, restartAdvertising = false;
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer*) override {
        portENTER_CRITICAL(&stateMux); linked=true; portEXIT_CRITICAL(&stateMux);
    }
    void onDisconnect(BLEServer*) override {
        portENTER_CRITICAL(&stateMux); linked=false; restartAdvertising=true; portEXIT_CRITICAL(&stateMux);
    }
} serverCallbacks;
class ReceiveCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* source) override {
        auto value = source->getValue();
        if (value.empty() || value.size() > 96 || !inbox) return;
        Message message{};
        for (size_t i=0;i<value.size();++i) {
            unsigned char c=value[i];
            message.text[i]=(c>=32 && c<=126)?c:' ';
        }
        // Keep only the latest notification. Rendering happens in the main loop.
        xQueueOverwrite(inbox, &message);
    }
} receiveCallbacks;
}
void BleLink::begin() {
    inbox = xQueueCreate(1, sizeof(Message));
    if (!inbox) { Serial.println("BLE queue allocation failed"); return; }
    BLEDevice::init(Config::bleName);
    BLEServer* server=BLEDevice::createServer();
    server->setCallbacks(&serverCallbacks);
    BLEService* service=server->createService(Config::serviceUuid);
    auto* rx=service->createCharacteristic(Config::rxUuid,BLECharacteristic::PROPERTY_WRITE);
    rx->setCallbacks(&receiveCallbacks);
    tx=service->createCharacteristic(Config::txUuid,BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_NOTIFY);
    tx->addDescriptor(new BLE2902());
    tx->setValue("Xpre-Enyx ready");
    service->start();
    auto* advertising=BLEDevice::getAdvertising();
    advertising->addServiceUUID(Config::serviceUuid);
    advertising->setScanResponse(true);
    BLEDevice::startAdvertising();
}
void BleLink::update() {
    portENTER_CRITICAL(&stateMux);
    bool restart=restartAdvertising; restartAdvertising=false;
    portEXIT_CRITICAL(&stateMux);
    if (restart) BLEDevice::startAdvertising();
}
bool BleLink::connected() {
    portENTER_CRITICAL(&stateMux); bool value=linked; portEXIT_CRITICAL(&stateMux);
    return value;
}
bool BleLink::takeMessage(char* output,size_t capacity) {
    Message message{};
    if (!inbox || !capacity || xQueueReceive(inbox,&message,0)!=pdTRUE) return false;
    strlcpy(output,message.text,capacity); return true;
}
void BleLink::publish(const char* text) {
    if (!tx) return;
    tx->setValue(std::string(text));
    if (connected()) tx->notify();
}
