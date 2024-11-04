#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid_Router = "Happy House Tang 2";
const char *password_Router = "2345678@";

const char *ssid_AP = "HỘP ĐÊM GAY 3000";
const char *password_AP = "12345678";

const char *mqtt_broker = "broker.emqx.io";
const char *topic = "esp32/test";
const char *mqtt_username = "demo";
const char *mqtt_password = "demo";
const int mqtt_port = 1883;

const char* apiKey = "";
String apiUrl = "https://api.openai.com/v1/chat/completions";
String finalPayload = "";

bool initialPrompt = true;
bool gettingResponse = true;

WiFiClient espClient;
PubSubClient client(espClient);
HTTPClient http;

void softAPConfig() {
  Serial.println("Setting soft-AP configuration ... ");
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  Serial.println("Setting soft-AP ... ");
  bool result = WiFi.softAP(ssid_AP, password_AP);

  if (result) {
    Serial.println("Ready");
    Serial.println(String("Soft-AP IP address = ") + WiFi.softAPIP().toString());
    Serial.println(String("MAC address = ") + WiFi.softAPmacAddress().c_str());
  } else {
    Serial.println("Failed!");
  }
}

void connectWifiConfig() {
  Serial.println("\nSetting Station configuration ... ");
  WiFi.begin(ssid_Router, password_Router);
  Serial.println(String("Connecting to ") + ssid_Router);

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 10000;

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected, IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Wi-Fi");
  }
  http.begin(apiUrl);
}

void setupMQTT() {
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    message += (char) payload[i];
  }
  Serial.println(message);
  Serial.println("-----------------------");
  
  chatGptCall(message);
}

void chatGptCall(String payload) {
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(apiKey));

  if (initialPrompt) {
    finalPayload = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \"" + payload + "\"}]}";
    initialPrompt = false;
  } else {
    finalPayload = finalPayload + ",{\"role\": \"user\", \"content\": \"" + payload + "\"}]}";
  }

  int httpResponseCode = http.POST(finalPayload);
  
  if (httpResponseCode == 200) {
    String response = http.getString();

    DynamicJsonDocument jsonDoc(2048);  // Increase buffer size if needed
    deserializeJson(jsonDoc, response);
    String outputText = jsonDoc["choices"][0]["message"]["content"];
    outputText.trim();
    Serial.print("CHATGPT: ");
    Serial.println(outputText);
    String returnResponse = "{\"role\": \"assistant\", \"content\": \"" + outputText + "\"}";

    finalPayload = removeEndOfString(finalPayload);
    finalPayload = finalPayload + "," + returnResponse;
    gettingResponse = false;

    client.publish(topic, outputText.c_str());
  } else {
    Serial.printf("Error %i\n", httpResponseCode);
  }
}

String removeEndOfString(String originalString) {
  int stringLength = originalString.length();
  String newString = originalString.substring(0, stringLength - 2);
  return newString;
}

void setup() {
  Serial.begin(115200);
  softAPConfig();
  connectWifiConfig();
  setupMQTT();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}
