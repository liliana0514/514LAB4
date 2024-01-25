#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <cfloat> // 用于 FLT_MAX

// 定义 BLE 服务和特征的 UUID
static BLEUUID serviceUUID("db0e37aa-c7ff-4584-936d-e39622848d33");
static BLEUUID charUUID("0fd8fa9f-34da-40bb-8cb7-afc7d0174389");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// 用于跟踪数据的全局变量
float maxDistance = -FLT_MAX;  // 初始化为非常低的值
float minDistance = FLT_MAX;   // 初始化为非常高的值

// 接收数据的回调
static void notifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {

    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(", data length: ");
    Serial.println(length);

    // 将接收到的数据转换为字符串
    String dataStr = "";
    for (int i = 0; i < length; i++) {
        dataStr += (char)pData[i];
    }

    // 提取距离值
    int startIndex = dataStr.indexOf(' ') + 1; // 找到第一个空格后的位置
    int endIndex = dataStr.indexOf(' ', startIndex); // 找到第二个空格的位置
    String distanceStr = dataStr.substring(startIndex, endIndex);

    // 将提取的字符串转换为浮点数
    float receivedDistance = distanceStr.toFloat();

    // 更新最大和最小距离
    if (receivedDistance > maxDistance) {
        maxDistance = receivedDistance;
    }
    if (receivedDistance < minDistance) {
        minDistance = receivedDistance;
    }

    // 打印当前距离，最大和最小距离
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

    // 其他任务...

    delay(1000); // 延迟一秒
}
