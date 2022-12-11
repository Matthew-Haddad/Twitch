/*
  ESP32_BLE_SERVER
  @author Heath Matthews
*/
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// I spent like 16 hours debugging this shit. 
// The UUIDs are not updated for some reason 
// on the ESP32 until I restart my laptop
static BLEUUID SERVER_UUID("79412f54-7945-11ed-a1eb-0242ac120002");
static BLEUUID ALERT_CHAR_UUID("bb6f0184-7941-11ed-a1eb-0242ac120002");
static BLEUUID VERIFY_CHAR_UUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
#define DEVICE_NAME "quake"

const int bufferLength = 8;
char alertBuffer[bufferLength]; //BLE characteristic values are byte arrays
char verifyBuffer[bufferLength];
bool deviceConnected = false;
BLECharacteristic *pAlert;
BLECharacteristic *pVerify;

/* This function handles the server callbacks */
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* MyServer) {
      deviceConnected = true;
      Serial.println("device connected.");
    };
    void onDisconnect(BLEServer* MyServer) {
      deviceConnected = false;
      Serial.println("device disconnected, advertising started.");
      BLEDevice::startAdvertising();
    }
};

// bool warningRecieved = false;
// class InputReceivedCallbacks: public BLECharacteristicCallbacks {
//     void onRead(BLECharacteristic *pAlertReadState) {
//         Serial.println("read.");
//         warningRecieved = true;
//     }
// };

void setup() {
  Serial.begin(9600);
  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVER_UUID);

  pAlert = pService->createCharacteristic(
                      ALERT_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ);
  pVerify = pService->createCharacteristic(
                      VERIFY_CHAR_UUID,
                      BLECharacteristic::PROPERTY_WRITE);

  pServer->setCallbacks(new ServerCallbacks());
  // pAlert->setCallbacks(new InputReceivedCallbacks());
  pService->start();

  BLEAdvertising *pAdvert = BLEDevice::getAdvertising();
  pAdvert->addServiceUUID(SERVER_UUID);
  pAdvert->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("done.");
}


void loop() {
  // sleep here
  delay(1000);
  // (1) TODO: replace this with wake from sleep on accelerometer threshold
  //delay(random(2000, 10000));
  // (1) END TODO.

  // (2) TODO: replace this with classification of data window
  float val = classify();

  // (2) END TODO.

  if (val >= 90.0) {
    Serial.printf("clonic activity suspected, sending warning...");
    sendWarning(val);
    Serial.println("warning recieved, going back to sleep.");
  }
}

/*
  sets warn value and waits for response
*/
void sendWarning(float val) {
  sprintf(alertBuffer, "%3.2f", val);
  pAlert->setValue(alertBuffer);
  
  while (memcmp(alertBuffer, verifyBuffer, bufferLength) != 0) {
    sprintf(verifyBuffer, "%3.2f", pAlert->getValue());
    delay(1000);
  }
  sprintf(alertBuffer, "");
  pAlert->setValue(alertBuffer);
}

/*
  this will be replaced by ML classifier
*/
float classify() {
  //testing
  float certainty = random(1000, 10000)/100.00;
  Serial.printf("certainty of clonic activity: %3.2f\n", certainty);
  return certainty;
}
