#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

char *ssid = "Vodafone-F24";
char *pwd = "Charger76";

#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "wE-LX6DkhxYPQ0GZlI6cYcMzV7lnP3nLiotexfDrf5nALQnsfu67rO8hVw5F74PCMxTA4fDYYf2P0JTbV8T2-A=="
#define INFLUXDB_ORG "479904ff12f20eea"
#define INFLUXDB_BUCKET "IoT48033"

#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 20
#define WRITE_BUFFER_SIZE MAX_BATCH_SIZE * 3

// Time zone info
#define TZ_INFO "AEST-10AEDT,M10.1.0,M4.1.0/3"

// Declare InfluxDB client_mqtt instance with preconfigured InfluxCloud certificate
InfluxDBClient client_influxdb(INFLUXDB_URL, INFLUXDB_ORG,
            INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
// Declare Data point

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

volatile int8_t ind_channel2scan = 1;
volatile int8_t thr = -80;
volatile int scan_time = 100; // in ms

unsigned long tA_clock_calibration = 0; 

String clientID;

WiFiClient espClient;
PubSubClient client_mqtt(espClient);

void callback_mqtt(char *topic, byte *payload, unsigned int length);
void reconnect();
void fun_subscribe(void);
void fun_connect2wifi();

void setup()
{
  Serial.begin(115200);
  fun_connect2wifi();

  clientID = "kaisesp32-";
  clientID += WiFi.macAddress();

  client_mqtt.setServer(mqtt_broker, mqtt_port);
  client_mqtt.setCallback(callback_mqtt);
  while (!client_mqtt.connected())
  {
    Serial.printf("The client_mqtt %s connects to the public MQTT broker\n", clientID.c_str());
    if (client_mqtt.connect(clientID.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Public EMQX MQTT broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client_mqtt.state());
      delay(2000);
    }
  }
  fun_subscribe();

  // Accurate time is necessary for certificate validation and writing in batches
  // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client_influxdb.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client_influxdb.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client_influxdb.getLastErrorMessage());
  }

  // Enable messages batching and retry buffer
  client_influxdb.setWriteOptions(
      WriteOptions().writePrecision(WRITE_PRECISION).
      batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));

  // Add tags to the data point
  // sensorNetworks.addTag("device", "ESP32");
  // sensorNetworks.addTag("location", "homeoffice");
  // sensorNetworks.addTag("esp32_id", String(WiFi.BSSIDstr().c_str()));
  tA_clock_calibration = millis(); 
}

void loop()
{
  if (millis() - tA_clock_calibration > 60*60*1000)
  {
    timeSync(TZ_INFO, "pool.ntp.org", "time.nist.gov");
    tA_clock_calibration = millis(); 
  }

  // If no Wifi signal, try to reconnect it
  if (!WiFi.isConnected())
  {
    WiFi.reconnect(); 
    Serial.println("Wifi connection lost");
  }
  
  if (!client_mqtt.connected())
  {
    reconnect();
  }
  Serial.println(client_mqtt.loop());

  int n = WiFi.scanNetworks(0, 0, 0, scan_time);
  time_t tnow = time(nullptr);

  if (n != 0)
  {
    int cnt = 0;
    for (int i = 0; i < n; i++)
    {
      if (WiFi.RSSI(i) >= thr)
      {
        Point sensorNetworks("wifi_scan");
        sensorNetworks.addTag("SSID", WiFi.SSID(i));
        sensorNetworks.addTag("channel", String(WiFi.channel(i)));
        sensorNetworks.addTag("MAC", WiFi.BSSIDstr());
        sensorNetworks.addTag("location", "homeoffice");
        sensorNetworks.addField("rssi", WiFi.RSSI(i));
        sensorNetworks.setTime(tnow); // set the time

        String tmp = client_influxdb.pointToLineProtocol(sensorNetworks);        
        Serial.print("Writing: ");
        Serial.println(tmp);

        // Write point into buffer - low priority measures
        client_influxdb.writePoint(sensorNetworks);

        String topic_base = String("kaisesp32/wifiScan/") + String(WiFi.channel(i)) + "/";

        tmp = String(WiFi.RSSI(i));
        String tmp1 = topic_base + WiFi.SSID(i);
        const char *payload = tmp.c_str();
        client_mqtt.publish((tmp1).c_str(), payload); 
        // delay(500); 
      }
    }    
  }
  else
  {
    Serial.println("Zero networks scanned ...");
  }
  
  // End of the iteration - force write of all the values into InfluxDB as single transaction
  if (!client_influxdb.isBufferEmpty())
  {
    Serial.println("Flushing data into InfluxDB");
    if (!client_influxdb.flushBuffer())
    {
      Serial.print("InfluxDB flush failed: ");
      Serial.println(client_influxdb.getLastErrorMessage());
      Serial.print("Full buffer: ");
      Serial.println(client_influxdb.isBufferFull() ? "Yes" : "No");
    }
  }

}

void callback_mqtt(char *topic, byte *payload, unsigned int length)
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
  while (!client_mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");

    if (client_mqtt.connect(clientID.c_str()))
    {
      Serial.println("connected");
      fun_subscribe();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client_mqtt.state());
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
    client_mqtt.subscribe(topicList[i]);
    Serial.print("Subscribed to: ");
    Serial.println(topicList[i]);
  }
}

void fun_connect2wifi()
{
  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n");
  Serial.print("Connected to the Wi-Fi network with local IP: ");
  Serial.println(WiFi.localIP());
}
