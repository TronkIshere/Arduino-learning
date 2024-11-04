#include <Arduino.h>

#include "WiFi.h"

#define pin_trigger 13
#define pin_echo 14

#define pin_red 0
#define pin_green 38
#define pin_blue 39
#define ch_red 0
#define ch_green 1
#define ch_blue 2

#define DC_MAX_RED 255
#define DC_MAX_GREEN 511
#define DC_MAX_BLUE 1023

const char rssi_thr0 = -20;
const char rssi_thr1 = -30;
const char rssi_thr2 = -50;
const char rssi_thr3 = -80;

volatile uint16_t dc_led = 50;

float fun_sonar(void);

void setup()
{
  pinMode(pin_echo, INPUT);
  pinMode(pin_trigger, OUTPUT);

  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Setup done");

  ledcSetup(ch_red, 1000, 8);
  ledcAttachPin(pin_red, ch_red);

  ledcSetup(ch_green, 1000, 9);
  ledcAttachPin(pin_green, ch_green);

  ledcSetup(ch_blue, 1000, 10);
  ledcAttachPin(pin_blue, ch_blue);
}

void loop()
{
  // Serial.println("scan start");
  // float range = fun_sonar();
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks(0, 0, 0, 100, 6, "Kai");
  // Serial.println("scan done");
  if (n != 0)
  {
    //   Serial.println("no networks found");
    // }
    // else
    // {
    // Serial.print(n);
    // Serial.println(" networks found");

    // String tmp;

    // for (int i = 0; i < n; ++i)
    // {
    //   tmp += WiFi.SSID(i);
    //   tmp += ": ";
    //   tmp += WiFi.BSSIDstr(i);
    //   tmp += "; Channel: ";
    //   tmp += String(WiFi.channel(i));
    //   tmp += "; RSSI: ";
    //   tmp += String(WiFi.RSSI(i));
    //   tmp += "dBm\n";
    // }
    // Serial.print(tmp);

    // Serial.printf(">range: %f\n", range);
    Serial.printf(">rssi: %d\n", WiFi.RSSI(0));
    Serial.printf(">dc: %d\n", dc_led);

    char wifi_rssi = WiFi.RSSI(0);
    if (wifi_rssi >= rssi_thr1)
    {
      if (wifi_rssi > rssi_thr0)
      {
        wifi_rssi = rssi_thr0;
      }
      dc_led = map(wifi_rssi, rssi_thr1, rssi_thr0, 0, DC_MAX_RED);
      ledcWrite(ch_red, dc_led);
      ledcWrite(ch_green, 0);
      ledcWrite(ch_blue, 0);
    }
    if (wifi_rssi < rssi_thr1 && wifi_rssi >= rssi_thr2)
    {
      dc_led = map(wifi_rssi, rssi_thr2, rssi_thr1, 0, DC_MAX_GREEN);
      ledcWrite(ch_red, 0);
      ledcWrite(ch_green, dc_led);
      ledcWrite(ch_blue, 0);
    }
    if (wifi_rssi < rssi_thr2 && wifi_rssi >= rssi_thr3)
    {
      dc_led = map(wifi_rssi, rssi_thr3, rssi_thr2, 0, DC_MAX_BLUE);
      ledcWrite(ch_red, 0);
      ledcWrite(ch_green, 0);
      ledcWrite(ch_blue, dc_led);
    }

    delay(200);
  }
}

float fun_sonar()
{
  digitalWrite(pin_trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin_trigger, LOW);

  unsigned long tmp = pulseIn(pin_echo, HIGH);
  float range = (float)tmp / 1.0e6 * 340. / 2. * 100.;
  return range;
}
