#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define pin_led_red 0
#define pin_led_green 38
#define pin_led_blue 39

char *ssid = "Vodafone-F24";
char *pwd = "Charger76";

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
// const char *mqtt_broker = "broker.hivemq.com";
const char *topic = "kaisesp32/wifiMeta/rssi";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

const char *topicList[] = {
    "kaisesp32/msgIn/wifiScanControl"
    // Add more topics as needed
};
const int topicCount = sizeof(topicList) / sizeof(topicList[0]);

volatile int8_t ind_channel2scan = 3;
volatile int8_t thr = -80;
volatile int scan_time = 100; // in ms

String clientID;

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void fun_subscribe(void);

void setup()
{
  pinMode(pin_led_red, OUTPUT);
  pinMode(pin_led_green, OUTPUT);
  pinMode(pin_led_blue, OUTPUT);

  Serial.begin(115200);

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n");
  Serial.print("Connected to the Wi-Fi network with local IP: ");
  Serial.println(WiFi.localIP());

  clientID = "kaisesp32-";
  clientID += WiFi.macAddress();

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected())
  {
    Serial.printf("The client %s connects to the public MQTT broker\n", clientID.c_str());
    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Public EMQX MQTT broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  fun_subscribe();
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  Serial.println(client.loop());

  int n = WiFi.scanNetworks(0, 0, 0, scan_time, ind_channel2scan);
  if (n != 0)
  {
    int cnt = 0;
    for (int i = 0; i < n; i++)
    {
      if (WiFi.RSSI(i) >= thr)
      {
        cnt++; 
        String topic_base = String("kaisesp32/wifiScan/Chn") + String(ind_channel2scan) + "/";

        String tmp = String(WiFi.RSSI(i));
        String tmp1 = topic_base + WiFi.SSID(i);
        const char *payload = tmp.c_str();
        if (client.publish((tmp1).c_str(), payload))
        {
          // Serial.println("publish okay");
          Serial.println(tmp1 + ":" + String(payload));
        }
        else
        {
          Serial.println("publish failed");
        }
        // delay(500);
      }
    }
    Serial.printf("Updated %d topics", cnt); 
  }
  else
  {
    Serial.println("Zero networks scanned ...");
  }
  delay(2000);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (!error)
  {
    const char *channel = doc["channel"];
    const char *thr_local = doc["thr"];
    const char *time_per_scan = doc["time_per_scan"]; // in ms

    Serial.println(channel);
    Serial.println(thr);
    Serial.println(time_per_scan);

    ind_channel2scan = String(channel).toInt();
    thr = String(thr_local).toInt();
    scan_time = String(time_per_scan).toInt();
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    if (client.connect(clientID.c_str()))
    {
      Serial.println("connected");
      fun_subscribe();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void fun_subscribe(void)
{
  for (int i = 0; i < topicCount; i++)
  {
    client.subscribe(topicList[i]);
    Serial.print("Subscribed to: ");
    Serial.println(topicList[i]);
  }
}
