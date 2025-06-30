#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>


//WiFi credentials
// const char* ssid = "BELL673";
// const char* password = "C24C561F";

const char* ssid = "Linksys2.4GHz";
const char* password = "qwerty12345678900987654321drtg6ed";

//MQTT broker (your Raspberry Pi IP)
const char* mqtt_server = "192.168.2.53";

//MQTT set up
WiFiClient espClient;
PubSubClient client(espClient);

//MQTT topic
const char* topic = "greenhouse/table1";

//Pin set up on the ESP32
const int capacitiveSensor1 = 34;
const int capacitiveSensor2 = 35;  // change this pin analog ADC1 like 33
const int capacitiveSensor3 = 32;  // change this pin analog ADC1 like 34
const int solenoidValve = 21;      // change this pin digital
const int fan = 22;                // change this pin digital
const int DHTPIN = 2; //pin for temperature and moisture sensor
const int DHTTYPE = DHT11;//type of our sensor (there is also the DHT12)
DHT tempSensor(DHTPIN, DHTTYPE); //creating the object

//Reading average (just a constant number)
const int numReadings = 10;

//Calibration of the sensor
const int dryValue1 = 3700;  // Sensor in dry air measured with the code capacitive sensor calibration
const int wetValue1 = 570;   // Calibration: sensor in water measured with the code capacitive sensor calibration
const int dryValue2 = 3700;
const int wetValue2 = 570;
const int dryValue3 = 3700;
const int wetValue3 = 570;

//Keeps track of whether solenoid is ON or OFF
bool solenoidState = false;
//Keeps track of wether fan is ON or OFF
bool fanState = false;

//Time tracking
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 5000;  // 5 seconds

//Wifi set up on the ESP32
void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 5000; // 5 seconds timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    // Optional: allow background processes like MQTT loop or other tasks
    delay(10);  // tiny non-blocking wait to prevent watchdog reset
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed.");
    // Optionally retry later or go into low-power mode
  }
}

// values for the reconnection function
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000; // 5 seconds

//If the ESP32 can connect to the WiFi on the first trial
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
      } else {
        Serial.print("MQTT failed, rc=");
        Serial.print(client.state());
        Serial.println(" will retry...");
      }
    }
  }
}

// ---- SENSOR FUNCTIONS ----

float readMoisture(int pin, int dryValue, int wetValue) {
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(pin);
    delayMicroseconds(100); // tiny delay for ADC stability
  }
  int avg = total / 10;
  
  Serial.print("Raw value: ");
  Serial.println(avg);

  // Convert to moisture percentage (invert: lower = wetter)
  float percent = mapFloat(avg, dryValue, wetValue, 0, 100); //need to reveiew this part
  Serial.print("Moisture : ");
  Serial.print(percent);
  Serial.println("%");
  
  return constrain(percent, 0, 100); // Clamp 0–100%
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float readTemperature() {
  float temp = tempSensor.readTemperature();
  if (isnan(temp)) {
    Serial.println("Failed to read temperature from DHT");
    return -999;
  }
  return temp;
}

float readHumidity() {
  float humidity = tempSensor.readHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed to read humidity from DHT");
    return -999;
  }
  return humidity;
}

// ---- CONTROL FUNCTIONS ----

void controlSolenoid(float avgMoisture) {
  if (avgMoisture < 30.0) {
    solenoidState = true;
  } else if (avgMoisture > 50.0) {
    solenoidState = false;
  }
  digitalWrite(solenoidValve, solenoidState ? HIGH : LOW);
}

void controlFan(float temp) {
  if (temp > 20) {  //ESp32 can tolerate between -40°C and 105°C
  digitalWrite(fan, fanState ? HIGH : LOW);
  }
}

void publishData(float temp, float hum, float m1, float m2, float m3, float avgMoisture) {
  String payload = "{";
  payload += "\"temperature\":" + String(temp, 1) + ",";
  payload += "\"humidity\":" + String(hum, 1) + ",";
  payload += "\"moisture1\":" + String(m1, 1) + ",";
  payload += "\"moisture2\":" + String(m2, 1) + ",";
  payload += "\"moisture3\":" + String(m3, 1) + ",";
  payload += "\"averageMoisture\":" + String(avgMoisture, 1) + ",";
  payload += "\"solenoid\":" + String(solenoidState ? "true" : "false") + ",";
  payload += "\"fan\":" + String(fanState ? "true" : "false");
  payload += "}";

  client.publish(topic, payload.c_str());
  Serial.println("Published: " + payload);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  pinMode(capacitiveSensor1, INPUT);
  pinMode(capacitiveSensor2, INPUT);
  pinMode(capacitiveSensor3, INPUT);
  pinMode(solenoidValve, OUTPUT);
  pinMode(fan, OUTPUT);

  tempSensor.begin();
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
  float avgMoisture = (m1 + m2 + m3) / 3;

  float temp = readTemperature();
  float hum = readHumidity();

  // Solenoid and Fan
  controlSolenoid(avgMoisture);
  controlFan(temp);

  // Publish data every 5 seconds
  if (millis() - lastPublishTime >= publishInterval) {
    publishData(temp, hum, m1, m2, m3, avgMoisture);
    lastPublishTime = millis();
  }
}
//   //*WiFi verification
//   if (!client.connected()) {
//     reconnect();
//   }
//   client.loop();
//   //*WiFi verification

//   //*Capacitive Sensor 1
//   long total1 = 0;

//   // Take multiple readings for averaging
//   for (int i = 0; i < numReadings; i++) {
//     total1 += analogRead(capacitiveSensor1);
//     delay(20);  // Small delay between readings
//   }

//   int averageValue1 = total1 / numReadings;

//   // Convert to moisture percentage (invert: lower = wetter)
//   float moisturePercent1 = map(averageValue1, dryValue1, wetValue1, 0, 100);
//   moisturePercent1 = constrain(moisturePercent1, 0, 100);  // Clamp 0–100%
//   //*Capacituve Sensor 1

//   //*Capacitive Sensor 2
//   long total2 = 0;

//   // Take multiple readings for averaging
//   for (int i = 0; i < numReadings; i++) {
//     total2 += analogRead(capacitiveSensor2);
//     delay(20);  // Small delay between readings
//   }

//   int averageValue2 = total2 / numReadings;

//   // Convert to moisture percentage (invert: lower = wetter)
//   float moisturePercent2 = map(averageValue2, dryValue2, wetValue2, 0, 100);
//   moisturePercent2 = constrain(moisturePercent2, 0, 100);  // Clamp 0–100%
//   //*Capacituve Sensor 2

//   //*Capacitive Sensor 3
//   long total3 = 0;

//   // Take multiple readings for averaging
//   for (int i = 0; i < numReadings; i++) {
//     total3 += analogRead(capacitiveSensor3);
//     delay(20);  // Small delay between readings
//   }

//   int averageValue3 = total3 / numReadings;

//   // Convert to moisture percentage (invert: lower = wetter)
//   float moisturePercent3 = map(averageValue3, dryValue3, wetValue3, 0, 100);
//   moisturePercent3 = constrain(moisturePercent3, 0, 100);  // Clamp 0–100%
//   //*Capacituve Sensor 3

//   //*Tempearture and humidity sensor in the box
//   //For temperature
//   long totalTemp = 0;

//   // Take multiple readings for averaging
//   for (int i = 0; i < numReadings; i++) {
//     totalTemp += tempSensor.readTemperature();
//     delay(20);  // Small delay between readings
//   }

//   int averageTemp = totalTemp / numReadings;

//   //For humidity
//   long totalHumidity = 0;

//   // Take multiple readings for averaging
//   for (int i = 0; i < numReadings; i++) {
//     totalHumidity += tempSensor.readHumidity();
//     delay(20);  // Small delay between readings
//   }

//   int averageHumidity = totalHumidity / numReadings;
//   //*Tempearture and humidity sensor in the box

//   //*Solenoid Valve
//   float averageMoisture = (moisturePercent1 + moisturePercent2 + moisturePercent3) / 3;

//   if (averageMoisture > 30.0) {
//     solenoidState = true;
//   } else if (averageMoisture > 50.0) {
//     solenoidState = false;
//   }

//   // Apply the state to the solenoid valve
//   if (solenoidState) {
//     digitalWrite(solenoidValve, HIGH);
//   } else {
//     digitalWrite(solenoidValve, LOW);
//   }

//   //*Solenoid Valve

//   //*Fan
//   if (averageTemp > 60) {  //ESp32 can tolerate between -40°C and 105°C
//     digitalWrite(fan, HIGH);
//   } else {
//     digitalWrite(fan, LOW);
//   }



//   // Convert to JSON string
//   String payload = "{\"temperature\":" + String(temp, 2) + "}";

//   // Publish
//   client.publish(topic, payload.c_str());
//   Serial.print("Published: ");
//   Serial.println(payload);

//   delay(5000);  // Publish every 5 seconds
// }
