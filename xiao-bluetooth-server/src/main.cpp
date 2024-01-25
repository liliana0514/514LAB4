#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

// HC-SR04 Pins
const int trigPin = 6;
const int echoPin = 43;

// BLE Server
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// Global variables for sensor readings and DSP
int rawDistance;
float filteredDistance;
const int numReadings = 10;
float readings[numReadings];
int readIndex = 0;
float total = 0;
float average = 0;

#define SERVICE_UUID        "db0e37aa-c7ff-4584-936d-e39622848d33"
#define CHARACTERISTIC_UUID "0fd8fa9f-34da-40bb-8cb7-afc7d0174389"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    for (int i = 0; i < numReadings; i++) {
        readings[i] = 0;
    }

    BLEDevice::init("XIAO_ESP32S3_liliana");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Connected! Device name: XIAO_ESP32S3_liliana");
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

float applyMovingAverage(float newReading) {
    total -= readings[readIndex];
    readings[readIndex] = newReading;
    total += readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;
    return total / numReadings;
}

void loop() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    rawDistance = duration * 0.034 / 2; // Distance in cm
    filteredDistance = applyMovingAverage(rawDistance);

    Serial.print("Raw Distance: ");
    Serial.print(rawDistance);
    Serial.print(" cm, Filtered Distance: ");
    Serial.print(filteredDistance);
    Serial.println(" cm");

    if (deviceConnected) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            if (filteredDistance < 30) {
                String bleMessage = "Distance: " + String(filteredDistance) + " cm";
                pCharacteristic->setValue(bleMessage.c_str());
                pCharacteristic->notify();
                Serial.println("Notify value: " + bleMessage);
            }
            previousMillis = currentMillis;
        }
    }

    // Handle BLE connection status
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(1000);
}
