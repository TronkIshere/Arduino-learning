#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define bleServerName "kaisesp32_smartHome"
#define SERVICE_UUID (BLEUUID((uint16_t)0x1819))

#define pin_red 0
#define pin_green 38
#define pin_blue 39

volatile bool flag_red = false;
volatile bool flag_green = false;
volatile bool flag_blue = false;

char res_red_pwm = 10;
char res_green_pwm = 10;
char res_blue_pwm = 10;

volatile uint8_t dc_red = 50;
volatile uint8_t dc_green = 50;
volatile uint8_t dc_blue = 50;

BLEServer *pServer = NULL;

BLECharacteristic redLEDChar("57b1d4f5-b274-49f3-84c4-f036594855e5",
                             BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
BLECharacteristic greenLEDChar("fdd82460-dafa-4a05-80a1-59639e56753e",
                               BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ | 
                               BLECharacteristic::PROPERTY_WRITE);
BLECharacteristic blueLEDChar("b5356dc8-1bdb-4337-a137-99ecdbaa420d",
                              BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic blueLEDControlChar("c5356dc8-1bdb-4337-a137-99ecdbaa420d",
                                     BLECharacteristic::PROPERTY_WRITE);

bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("Device Connected");
  };
  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("Device Disconnected");
  }
};

class RedLEDCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      flag_red = String(value[0]).toInt() != 0;
      if (value.length() > 1)
      {
        dc_red = std::stoi(value.substr(1));
      }
    }
  }
};

class GreenLEDCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      flag_green = String(value[0]).toInt() != 0;
      if (value.length() > 1)
      {
        dc_green = std::stoi(value.substr(1));
      }
    }
  }
};

class BlueLEDControlCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      flag_blue = String(value[0]).toInt() != 0;
      if (value.length() > 1)
      {
        dc_blue = std::stoi(value.substr(1));
      }
    }
  }
};

void setup()
{
  Serial.begin(115200);

  // Setup PWM for LEDs
  ledcSetup(0, 1000, 8);
  ledcAttachPin(pin_red, 0);
  ledcSetup(1, 1000, 8);
  ledcAttachPin(pin_green, 1);
  ledcSetup(2, 1000, 8);
  ledcAttachPin(pin_blue, 2);

  BLEDevice::init(bleServerName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *homeService = pServer->createService(SERVICE_UUID);

  homeService->addCharacteristic(&redLEDChar);
  redLEDChar.setCallbacks(new RedLEDCallbacks());

  homeService->addCharacteristic(&greenLEDChar);
  greenLEDChar.setCallbacks(new GreenLEDCallbacks());

  homeService->addCharacteristic(&blueLEDChar);
  homeService->addCharacteristic(&blueLEDControlChar);
  blueLEDControlChar.setCallbacks(new BlueLEDControlCallbacks());

  // Add Descriptors
  BLEDescriptor *pDescriptor_redLED = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pDescriptor_redLED->setValue("Red LED Control");
  redLEDChar.addDescriptor(pDescriptor_redLED);

  BLEDescriptor *pDescriptor_greenLED = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pDescriptor_greenLED->setValue("Green LED Control");
  greenLEDChar.addDescriptor(pDescriptor_greenLED);

  BLEDescriptor *pDescriptor_blueLED = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pDescriptor_blueLED->setValue("Blue LED Status");
  blueLEDChar.addDescriptor(pDescriptor_blueLED);

  BLEDescriptor *pDescriptor_blueLEDControl = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pDescriptor_blueLEDControl->setValue("Blue LED Control");
  blueLEDControlChar.addDescriptor(pDescriptor_blueLEDControl);

  homeService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a client connection to notify...");
}

void loop()
{
  if (deviceConnected)
  {
    ledcWrite(0, map(flag_red, 0, 100, 0, pow(2, res_red_pwm) - 1) ? dc_red : 0);
    ledcWrite(1, map(flag_green, 0, 100, 0, pow(2, res_green_pwm) - 1) ? dc_green : 0);
    ledcWrite(2, map(flag_blue, 0, 100, 0, pow(2, res_blue_pwm) - 1) ? dc_blue : 0);

    String redStatus = "Red LED: " + String(flag_red) + "; DC(%): " + String(dc_red);
    String greenStatus = "Green LED: " + String(flag_green) + "; DC(%): " + String(dc_green);
    String blueStatus = "Blue LED: " + String(flag_blue) + "; DC(%): " + String(dc_blue);

    redLEDChar.setValue(redStatus.c_str());
    redLEDChar.notify();
    greenLEDChar.setValue(greenStatus.c_str());
    greenLEDChar.notify();
    blueLEDChar.setValue(blueStatus.c_str());
    blueLEDChar.notify();

    Serial.println(redStatus + " \n" + greenStatus + " \n" + blueStatus);
    delay(2000);
  }
  else
  {
    pServer->startAdvertising();
    Serial.println("Waiting for a client connection to notify...");
    while (!deviceConnected)
    {
      Serial.print(".");
      delay(1000);
    }
  }
}
