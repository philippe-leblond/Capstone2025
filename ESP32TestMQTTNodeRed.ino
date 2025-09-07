#include <WiFi.h>
#include <PubSubClient.h>

// Wi-Fi credentials
const char* ssid = "Scooby-Doo";
const char* password = "Hakuna-matata";

// MQTT broker (your Raspberry Pi IP)
const char* mqtt_server = "192.168.0.64";

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT topic
const char* topic = "greenhouse/table1";

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Dummy sensor value
  float temp = 23.5 + random(-10, 10) * 0.1;

  // Convert to JSON string
  String payload = "{\"temperature\":" + String(temp, 2) + "}";

  // Publish
  client.publish(topic, payload.c_str());
  Serial.print("Published: ");
  Serial.println(payload);

  delay(5000); // Publish every 5 seconds
}
