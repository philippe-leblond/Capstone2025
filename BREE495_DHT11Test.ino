#include <DHT.h>

// --- DHT Sensor Setup ---
const int DHTPIN = 2;
const int DHTTYPE = DHT11;
DHT tempSensor(DHTPIN, DHTTYPE);

float readTemperature() {
  float temp = tempSensor.readTemperature();
  if (isnan(temp)) {
    Serial.println("Failed to read temperature from DHT");
    return -999;
  }
  return temp;
}

float readHumidity() {
  float hum = tempSensor.readHumidity();
  if (isnan(hum)) {
    Serial.println("Failed to read humidity from DHT");
    return -999;
  }
  return hum;
}

void setup() {
  Serial.begin(115200);
  tempSensor.begin();
}

void loop() {
  float temp = readTemperature();
  float hum = readHumidity();

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" °C  |  Humidity: ");
  Serial.print(hum);
  Serial.println(" %");

  delay(2000);
}
