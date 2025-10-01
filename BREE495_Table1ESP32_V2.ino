#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_NAU7802.h>
#include <Preferences.h>
#include <Wire.h>

#define TCAADDR 0x70

//WiFi credentials
const char* ssid = "BELL673"; //Serre
const char* password = "C24C561F";

//MQTT broker (your Raspberry Pi IP)
const char* mqtt_server = "192.168.2.16"; //Serre

//MQTT set up
WiFiClient espClient;
PubSubClient client(espClient);

//MQTT topic
const char* topicData = "greenhouse/table1/data";
const char* topicCommand = "greenhouse/table1/command";

//Pin set up on the ESP32
const int capacitiveSensor1 = 34;
const int capacitiveSensor2 = 35;
const int capacitiveSensor3 = 32;
const int solenoidValve = 19; // Change circuit with the new pin 
const int fan = 18; // Change circuit with the new pin
const int DHTPIN = 2;
const int DHTTYPE = DHT11;
DHT tempSensor(DHTPIN, DHTTYPE);

// Create separate instances for each NAU7802
Adafruit_NAU7802 nau1;
Adafruit_NAU7802 nau2;
Adafruit_NAU7802 nau3;

// Preferences setup to access the deep memory of ESP32
Preferences prefs;

// Store offset and scale factor for each load cell
long offset[3];
float scale_factor[3];


//Reading average (just a constant number)
const int numReadings = 10;

//Calibration of the sensor
const int dryValue1 = 3700;
const int wetValue1 = 570;
const int dryValue2 = 3700;
const int wetValue2 = 570;
const int dryValue3 = 3700;
const int wetValue3 = 570;


// Keeps track of whether solenoid is ON or OFF
bool solenoidState = false;
// Keeps track of whether fan is ON or OFF
bool fanState = false;
// Keeps track of whether solenoid is manual or automatic
bool manualOverride = false; // false = automatic, true = manual

// To allow thresholdSwitch

// Moisture threshold used in automatic mode (percent 0-100)
float moistureThreshold = 30.0; // default — can be updated via MQTT

// Time tracking
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 15000;  // 15 seconds

//Wifi set up on the ESP32
void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 5000; // 5 seconds timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(10);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed.");
  }
}

// values for the reconnection function
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000; // 5 seconds

void reconnect() {
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = now;

      String clientId = "ESP32Client-" + String(WiFi.macAddress());
      Serial.print("Attempting MQTT connection with client ID: ");
      Serial.println(clientId);

      if (client.connect(clientId.c_str())) {
        Serial.println("MQTT connected");

        // Subscribe to topicCommand (and optionally to topicData if desired)
        if (client.subscribe(topicCommand)) {
          Serial.print("Subscribed to topicCommand: ");
          Serial.println(topicCommand);
        } else {
          Serial.println("Failed to subscribe to topicCommand");
        }
      } else {
        Serial.print("MQTT failed, rc=");
        Serial.print(client.state());
      }
    }
  }
}

// ---- SENSOR FUNCTIONS ----

float readMoisture(int pin, int dryValue, int wetValue) {
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(pin);
    delayMicroseconds(100);
  }
  int avg = total / 10;

  float percent = mapFloat(avg, dryValue, wetValue, 0, 100);
  return constrain(percent, 0, 100);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float readTemperature() {
  float temp = tempSensor.readTemperature();
  if (isnan(temp)) {
    // Serial.println("Failed to read temperature from DHT");
    return -999;
  }
  return temp;
}

float readHumidity() {
  float humidity = tempSensor.readHumidity();
  if (isnan(humidity)) {
    // Serial.println("Failed to read humidity from DHT");
    return -999;
  }
  return humidity;
}

// Helper function for changing TCA output channel for the I2C multiplexer
void tcaselect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// ---- CONTROL FUNCTIONS ----

// Automatic control uses the single threshold: if avgMoisture < threshold -> ON, else OFF
void controlSolenoid(float avgMoisture) {
  if (manualOverride) {
    // manual mode: do not change solenoidState here (it's controlled by MQTT manual commands)
    digitalWrite(solenoidValve, solenoidState ? HIGH : LOW);
    return;
  }

  // Automatic behaviour using the single threshold provided via MQTT or default
  if (avgMoisture < moistureThreshold) {
    solenoidState = true;
  } else {
    solenoidState = false;
  }

  digitalWrite(solenoidValve, solenoidState ? HIGH : LOW);
}

void controlFan(float temp) {
  if (temp == -999 || isnan(temp)) return;

  if (temp >= 25.0) {
    fanState = true;
  } else if (temp <= 23.0) {
    fanState = false;
  }

  digitalWrite(fan, fanState ? HIGH : LOW);
}

void publishData(float temp, float hum, float m1, float m2, float m3, float avgMoisture, float grams1, float grams2, float grams3, float avgWeight) {
  String payload = "{";
  payload += "\"temperature\":" + String(temp, 1) + ",";
  payload += "\"humidity\":" + String(hum, 1) + ",";
  payload += "\"moisture1\":" + String(m1, 1) + ",";
  payload += "\"moisture2\":" + String(m2, 1) + ",";
  payload += "\"moisture3\":" + String(m3, 1) + ",";
  payload += "\"averageMoisture\":" + String(avgMoisture, 1) + ",";
  payload += "\"weight1\":"+ String(grams1, 1) + ",";
  payload += "\"weight2\":"+ String(grams2, 1) + ",";
  payload += "\"weight3\":"+ String(grams3, 1) + ",";
  payload += "\"averageWeight\":"+ String(avgWeight, 1) + ",";
  payload += "\"moistureThreshold\":" + String(moistureThreshold, 1) + ",";
  payload += "\"solenoid\":" + String(solenoidState ? "true" : "false") + ",";
  payload += "\"fan\":" + String(fanState ? "true" : "false");
  payload += "}";

  client.publish(topicData, payload.c_str());
  Serial.println("Published: " + payload);
}

// helper: check if a string is a valid numeric value
bool isNumericString(const String &s) {
  bool seenDecimal = false;
  int start = 0;
  if (s.length() == 0) return false;
  if (s[0] == '+' || s[0] == '-') start = 1;
  for (int i = start; i < s.length(); i++) {
    char c = s[i];
    if (c == '.') {
      if (seenDecimal) return false;
      seenDecimal = true;
      continue;
    }
    if (c < '0' || c > '9') return false;
  }
  return (start < s.length()); // ensure there's at least one digit
}

// MQTT message callback to receive the message from Node Red
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  Serial.print("Message: ");
  Serial.println(message);

  // manual open
  if (message.equalsIgnoreCase("open")) {
    manualOverride = true;
    solenoidState = true;
    Serial.println("Solenoid manually opened via MQTT");
  }
  // manual stop
  else if (message.equalsIgnoreCase("stop")) {
    manualOverride = true;
    solenoidState = false;
    Serial.println("Solenoid manually stopped via MQTT");
  }
  // switch to automatic mode (use existing threshold)
  else if (message.equalsIgnoreCase("auto")) {
    manualOverride = false;
    Serial.println("Solenoid set to automatic mode (using threshold = " + String(moistureThreshold, 1) + "%)");
  }
  // if numeric payload -> set threshold and enable automatic mode
  else if (isNumericString(message)) {
    float newThreshold = message.toFloat();
    // clamp threshold to 0-100
    if (newThreshold < 0.0) newThreshold = 0.0;
    if (newThreshold > 100.0) newThreshold = 100.0;
    moistureThreshold = newThreshold;
    manualOverride = false; // set to automatic so the threshold is used immediately
    Serial.println("Moisture threshold set via MQTT to: " + String(moistureThreshold, 1) + "% (automatic mode)");
  }
  else {
    Serial.println("Unknown command received via MQTT");
  }

  // apply solenoid state right away if in manual mode (or keep as-is if automatic)
  digitalWrite(solenoidValve, solenoidState ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(capacitiveSensor1, INPUT);
  pinMode(capacitiveSensor2, INPUT);
  pinMode(capacitiveSensor3, INPUT);
  pinMode(solenoidValve, OUTPUT);
  pinMode(fan, OUTPUT);

  // Ensure outputs start in known state
  digitalWrite(solenoidValve, solenoidState ? HIGH : LOW);
  digitalWrite(fan, fanState ? HIGH : LOW);

  tempSensor.begin();

  // Start the NAU7802 loadcell sensor 
  Wire.begin(21, 22);
  tcaselect(0); 
  tcaselect(1);
  tcaselect(2);  

  prefs.begin("loadcells", true); // namespace "loadcells"

  // Getting the values form the calibration in the deep memory of the ESP32
 for (int i = 0; i < 3; i++) {
    offset[i]       = prefs.getLong(("offset" + String(i)).c_str(), 0);
    scale_factor[i] = prefs.getFloat(("scale" + String(i)).c_str(), 0);
  }


  


  tcaselect(0);
  if (!nau1.begin()) Serial.println("NAU7802 #1 not found!");

  tcaselect(1);
  if (!nau2.begin()) Serial.println("NAU7802 #2 not found!");

  tcaselect(2);
  if (!nau3.begin()) Serial.println("NAU7802 #3 not found!");


}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read sensors
  float m1 = readMoisture(capacitiveSensor1, dryValue1, wetValue1);
  float m2 = readMoisture(capacitiveSensor2, dryValue2, wetValue2);
  float m3 = readMoisture(capacitiveSensor3, dryValue3, wetValue3);
  float avgMoisture = (m1 + m2 + m3) / 3.0;

  float temp = readTemperature();
  float hum = readHumidity();

  float grams1 = 0;
  float grams2 = 0;
  float grams3 = 0;

  // Sensor 1 loadcell
  tcaselect(0);
  delay(3);
  while (!nau1.available()) delay(1); // wait until conversion is ready
  long raw1 = nau1.read();
  grams1 = (raw1 - offset[0]) / scale_factor[0];


  // Sensor 2 loadcell
  tcaselect(1);
  delay(3);
  while (!nau2.available()) delay(1); // wait until conversion is ready
  long raw2 = nau2.read();
  grams2 = (raw2 - offset[1]) / scale_factor[1];


  // Sensor 3 loadcell
  tcaselect(2);
  delay(3);
  while (!nau3.available()) delay(1); // wait until conversion is ready
  long raw3 = nau3.read();
  grams3 = (raw3 - offset[2]) / scale_factor[2];
  
  // Average of the three sensors
  float avgWeight = (grams1 + grams2 + grams3) / 3.0;
  
  // Solenoid and Fan
  controlSolenoid(avgMoisture);
  controlFan(temp);

  // Publish data every publishInterval
  if (millis() - lastPublishTime >= publishInterval) {
    publishData(temp, hum, m1, m2, m3, avgMoisture, grams1, grams2, grams3, avgWeight);
    lastPublishTime = millis();
  }
}
