#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define FORCE_PIN 34

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; }
  void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

void setup() {
  Serial.begin(115200);

  BLEDevice::init("ESP32_FSR");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  Serial.println("BLE started, waiting for connection...");
}

void loop() {
  if (deviceConnected) {
    int analogReading = analogRead(FORCE_PIN);

    String pressure = "";
    if (analogReading < 10)
      pressure = "No Pressure";
    else if (analogReading < 200)
      pressure = "Light Touch";
    else if (analogReading < 500)
      pressure = "Light Squeeze";
    else if (analogReading < 800)
      pressure = "Medium Squeeze";
    else
      pressure = "Big Squeeze";

    String message = "Raw: " + String(analogReading) + " - " + pressure;
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();

    Serial.println(message);
    delay(500);
  }
}