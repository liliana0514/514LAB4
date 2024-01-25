#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <cfloat> // for FLT_MAX

// Define the UUIDs for BLE service and characteristic
static BLEUUID serviceUUID("db0e37aa-c7ff-4584-936d-e39622848d33");
static BLEUUID charUUID("0fd8fa9f-34da-40bb-8cb7-afc7d0174389");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Global variables for tracking data
float maxDistance = -FLT_MAX;  // Initialize to a very low value
float minDistance = FLT_MAX;   // Initialize to a very high value

// Callback for receiving data
static void notifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {

    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(", data length: ");
    Serial.println(length);

    // Convert the received data to a string
    String dataStr = "";
    for (int i = 0; i < length; i++) {
        dataStr += (char)pData[i];
    }

    // Extract the distance value
    int startIndex = dataStr.indexOf(' ') + 1; // Find the position after the first space
    int endIndex = dataStr.indexOf(' ', startIndex); // Find the position of the second space
    String distanceStr = dataStr.substring(startIndex, endIndex);

    // Convert the extracted string to a floating-point number
    float receivedDistance = distanceStr.toFloat();

    // Update the maximum and minimum distances
    if (receivedDistance > maxDistance) {
        maxDistance = receivedDistance;
    }
    if (receivedDistance < minDistance) {
        minDistance = receivedDistance;
    }

    // Print the current distance, maximum and minimum distances
    Serial.print("Current Distance: ");
    Serial.println(receivedDistance);
    Serial.print("Max Distance: ");
    Serial.println(maxDistance);
    Serial.print("Min Distance: ");
    Serial.println(minDistance);
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }

    void onDisconnect(BLEClient* pclient) {
        connected = false;
        Serial.println("onDisconnect");
    }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);
    Serial.println(" - Connected to server");

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our characteristic");

    if (pRemoteCharacteristic->canRead()) {
        std::string value = pRemoteCharacteristic->readValue();
        Serial.print("The characteristic value was: ");
        Serial.println(value.c_str());
    }

    if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("");

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

void loop() {
    if (doConnect == true) {
        if (connectToServer()) {
            Serial.println("We are now connected to the BLE Server.");
        } else {
            Serial.println("We have failed to connect to the server; there is nothing more we will do.");
        }
        doConnect = false;
    }

    // Other tasks...

    delay(1000); // Delay for one second
}
