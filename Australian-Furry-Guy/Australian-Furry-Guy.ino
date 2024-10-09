// Lý thuyết:
// MQTT (Message Queueing Telemetry Transport) là một giao thức mạng kích thước nhỏ (lightweight), 
// hoạt động theo cơ chế publish – subscribe (tạm dịch: xuất bản – đăng ký) theo tiêu chuẩn ISO (ISO/IEC 20922) 
// và OASIS mở để truyền tin nhắn giữa các thiết bị.

// MQTT Broker hay máy chủ mô giới được coi như trung tâm, nó là điểm giao của tất cả các kết nối 
// đến từ Client (Publisher/Subscriber).

#include <WiFi.h>
#include <PubSubClient.h>

// Đây là đoạn code dùng để truy cập wifi
const char *ssid_Router = "Happy House Tang 2"; // Nhập tên Wi-Fi
const char *password_Router = "2345678@";       // Nhập mật khẩu Wi-Fi

// Đây là đoạn code dùng để tạo mạng wifi riêng
const char *ssid_AP = "Hộp đêm gay";           // Nhập tên AP
const char *password_AP = "12345678";          // Nhập mật khẩu AP

// config MQTT
const char *mqtt_broker = "broker.emqx.io"; // Đây là đại chỉ của MQTT broker
const char *topic = "esp32/test"; // đây là topic để ESP thấy và gửi dữ liệu qua MQTT
const char *mqtt_username = "demo"; // đây là tên người dùng xác thực cho giao thức MQTT
const char *mqtt_password = "demo"; // đây là mật khẩu dùng xác thực cho giao thức MQTT
const int mqtt_port = 1883; // cổng kết nối tới MQTT

WiFiClient espClient; // tạo ra đối tượng cho phép esp32 giao tiếp với mạng wifi
PubSubClient client(espClient); // tạo một đối tượng PubSubClient sử dụng espClient để kết nối tới MQTT broker

void setup() {
  Serial.begin(115200); // khởi tạo giao tiếp Serial với tốc độ truyền là 115200 bps
  softAPConfig(); // thiết lập chế độ AP cho ESP32
  connectWifiConfig(); // kết nối ESP32 với Wi-Fi qua router
  setupMQTT(); // thiết lập kết nối với MQTT broker
}

void loop() {
  //  kiểm tra kết nối với MQTT broker 
  // nếu không kết nối được, hàm reconnectMQTT() sẽ được gọi
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop(); // xử lý các sự kiện liên quan đến MQTT
}

void softAPConfig() {
  Serial.println("Setting soft-AP configuration ... ");
  WiFi.disconnect(); // ngắt kết nối wifi
  WiFi.mode(WIFI_AP_STA); // thiết lập ESP32 hoạt động ở chế độ AP và STA

  Serial.println("Setting soft-AP ... ");
  boolean result = WiFi.softAP(ssid_AP, password_AP); // tạo điểm truy cập wifi mới

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
  WiFi.begin(ssid_Router, password_Router); // Kết nối với router wifi
  Serial.println(String("Connecting to ") + ssid_Router);
  
  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 10000; 
  
  // kiểm tra kết nối wifi
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
}

void setupMQTT() {
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
}

void reconnectMQTT() {
  // Kiểm tra xem nó kết nối được chưa, nếu chưa thì thử lại sau mỗi 5 giây
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
  // Hàm này in ra tin nhắn nhận được
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}
