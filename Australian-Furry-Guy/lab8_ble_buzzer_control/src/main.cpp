#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define bleServerName "kaisesp32_buzzercontrol"
#define SERVICE_UUID "3ac583b1-701e-401c-a1d4-2e1601a7ebe0"
#define CHARACTERISTIC_UUID_ONOFF "3660f667-78bf-446f-a6e1-651f11ca7c4d"
#define CHARACTERISTIC_UUID_VOL "e7127f2c-f3aa-4ad2-9e0e-c67fd174ca31"
#define CHARACTERISTIC_UUID_FREQ "9a692014-6c7e-44f3-a6e1-8400f1ce9504"

bool deviceConnected = false;

// GPIO pin connected to the buzzer
const char buzzerPin = 9; // example pin, make sure it supports PWM

// PWM settings
const char pwmChannel = 0;
const int pwmFreq = 1000; // Default frequency in Hz
const char pwmResolution = 11;

// Volume levels: 0 (off), 127 (medium), 255 (high)
uint8_t volumeLevel = 0;
// Frequency levels: 1000 Hz, 2000 Hz, 4000 Hz
int frequencyLevels[] = {1000, 2000, 4000};

bool buzzerOn = false;    // Buzzer state (on/off)
int currentFrequency = 0; // Current frequency level index

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicOnOff;
BLECharacteristic *pCharacteristicVol;
BLECharacteristic *pCharacteristicFreq;

class ServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("Device Connected");
  }

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("Device Disconnected");
    // Restart advertising
    pServer->getAdvertising()->start();
    Serial.println("Restarted advertising");
  }
};

class BuzzerCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (pCharacteristic == pCharacteristicOnOff)
    {
      if (value == "1")
      {
        buzzerOn = true;
        ledcChangeFrequency(pwmChannel, frequencyLevels[currentFrequency], pwmResolution);
        ledcWrite(pwmChannel, map(constrain(volumeLevel, 0, 2), 0, 2, 10, pow(2, pwmResolution) - 1));
      }
      else
      {
        buzzerOn = false;
        ledcWrite(pwmChannel, 0);
      }
    }
    else if (pCharacteristic == pCharacteristicVol)
    {
      volumeLevel = atoi(value.c_str());
      if (buzzerOn)
      {
        ledcWrite(pwmChannel, map(constrain(volumeLevel, 0, 2), 0, 2, 10, (int)(pow(2, pwmResolution)*0.5)));
      }
    }
    else if (pCharacteristic == pCharacteristicFreq)
    {
      currentFrequency = atoi(value.c_str());
      if (buzzerOn)
      {
        ledcChangeFrequency(pwmChannel, frequencyLevels[currentFrequency], pwmResolution);
      }
    }
  }
};

void setup()
{
  Serial.begin(115200);
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(buzzerPin, pwmChannel);

  BLEDevice::init(bleServerName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks()); 

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristicOnOff = pService->createCharacteristic(CHARACTERISTIC_UUID_ONOFF, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicVol = pService->createCharacteristic(CHARACTERISTIC_UUID_VOL, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicFreq = pService->createCharacteristic(CHARACTERISTIC_UUID_FREQ, BLECharacteristic::PROPERTY_WRITE);

  pCharacteristicOnOff->setCallbacks(new BuzzerCallbacks());
  pCharacteristicVol->setCallbacks(new BuzzerCallbacks());
  pCharacteristicFreq->setCallbacks(new BuzzerCallbacks());

  BLEDescriptor *pDescriptor_onoff = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pDescriptor_onoff->setValue("On/Off");
  pCharacteristicOnOff->addDescriptor(pDescriptor_onoff);

  BLEDescriptor *pDescriptor_freq = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pDescriptor_freq->setValue("Frequency option (0-2)");
  pCharacteristicFreq->addDescriptor(pDescriptor_freq);

  BLEDescriptor *pDescriptor_vol = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pDescriptor_vol->setValue("Vol option (0-2)");
  pCharacteristicVol->addDescriptor(pDescriptor_vol);

  pService->start();

  pServer->getAdvertising()->start();

  Serial.println("Please connect to me...");
}

void loop()
{
  if (deviceConnected)
  {
    String tmp = "Buzzer status: " + String(buzzerOn) + "; frequency is: " +
                 String(frequencyLevels[currentFrequency]) + "; volume level is: " +
                 String(volumeLevel);
    Serial.println(tmp);
    delay(1000);
  }
}
