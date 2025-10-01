#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char *ssid = "Scooby-Doo"; // Enter your Wi-Fi name
const char *password = "Hakuna-matata";  // Enter Wi-Fi password


// MQTT Broker
const char *mqtt_broker = "192.168.0.56";
const char *topic = "emqx/esp32";
const char *mqtt_username = "pat";
const char *mqtt_password = "orchid";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastTime = 0;  // Stores the last time a message was sent
const long interval = 5000;   // Interval for sending message (5000 ms = 5 seconds)

void setup() {
    // Set software serial baud to 115200;
    Serial.begin(115200);
    // Connecting to a WiFi network
    WiFi.begin(ssid, password);
    Serial.println("Connected");
     // Connecting to Wi-Fi using WPA2-Enterprise (EAP-PEAP)
    // WiFi.disconnect(true); // Disconnect any existing Wi-Fi connections
    // WiFi.begin(ssid, password); // Use the WPA2 password to connect

    // If WPA2-Enterprise is required, you should use WiFi.begin with specific credentials for WPA2-Enterprise
    // For now, assume McGill uses EAP-PEAP for WPA2-Enterprise authentication
    //WiFi.begin(ssid, username, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the Wi-Fi network");
    //connecting to a mqtt broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    while (!client.connected()) {
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Public EMQX MQTT broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
    // Publish and subscribe
    //client.publish(topic, "Hi, I'm ESP32 ^^");
    client.subscribe(topic);
}

void loop() {
  client.loop();

  // Check if 5 seconds have passed
  if (millis() - lastTime >= interval) {
    // Send the message
    client.publish(topic, "Hi, I'm ESP32");
    
    // Update lastTime to the current time
    lastTime = millis();
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}
