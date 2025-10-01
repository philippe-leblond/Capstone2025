
#include <Adafruit_NAU7802.h>

#define TCAADDR 0x70

// Create separate instances for each NAU7802
Adafruit_NAU7802 nau1;
Adafruit_NAU7802 nau2;
Adafruit_NAU7802 nau3;
// Adafruit_NAU7802 nau4;

// Store offset and scale factor for each load cell
long offset[4];
float scale_factor[4];

// Helper function for changing TCA output channel
void tcaselect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// Function to calibrate (tare + known weight) for one NAU7802
void calibrateSensor(Adafruit_NAU7802 &nau, uint8_t channel, int sensorIndex, float knownWeight) {
  tcaselect(channel);

  Serial.print("Calibrating NAU7802 #");
  Serial.println(sensorIndex + 1);

  delay(1000);
  while (!nau.available()) delay(1);
  offset[sensorIndex] = nau.read();

  Serial.print("Offset (tare) for sensor ");
  Serial.print(sensorIndex + 1);
  Serial.print(" = ");
  Serial.println(offset[sensorIndex]);

  Serial.print("Place ");
  Serial.print(knownWeight);
  Serial.println(" g weight...");
  delay(5000); // give time to put the weight

  while (!nau.available()) delay(1);
  long raw_with_weight = nau.read();

  scale_factor[sensorIndex] = (float)(raw_with_weight - offset[sensorIndex]) / knownWeight;

  Serial.print("Scale factor for sensor ");
  Serial.print(sensorIndex + 1);
  Serial.print(" = ");
  Serial.println(scale_factor[sensorIndex]);
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Four NAU7802 with Calibration Example"));

  Wire.begin(21, 22);

  // Initialize each NAU7802
  tcaselect(0);
  if (!nau1.begin()) {
    Serial.println("Failed to find NAU7802 #1");
  }

  tcaselect(1);
  if (!nau2.begin()) {
    Serial.println("Failed to find NAU7802 #2");
  }

  tcaselect(2);
  if (!nau3.begin()) {
    Serial.println("Failed to find NAU7802 #3");
  }

  // tcaselect(3);
  // if (!nau4.begin()) {
  //   Serial.println("Failed to find NAU7802 #4");
  // }

  // Calibrate each sensor with a known weight (example: 496 g)
  calibrateSensor(nau1, 0, 0, 500.0);
  calibrateSensor(nau2, 1, 1, 100.0);
  calibrateSensor(nau3, 2, 2, 100.0);
  // calibrateSensor(nau4, 3, 3, 496.0);

  Serial.println("Calibration complete!");
}

void loop() {
  long raw;
  float grams;

  Serial.println("------------------------------------");

  // Sensor 1
  tcaselect(0);
  if (nau1.available()) {
    raw = nau1.read();
    grams = (raw - offset[0]) / scale_factor[0];
    Serial.print("Weight #1: ");
    Serial.print(grams);
    Serial.println(" g");
  }

  // Sensor 2
  tcaselect(1);
  if (nau2.available()) {
    raw = nau2.read();
    grams = (raw - offset[1]) / scale_factor[1];
    Serial.print("Weight #2: ");
    Serial.print(grams);
    Serial.println(" g");
  }

  // Sensor 3
  tcaselect(2);
  if (nau3.available()) {
    raw = nau3.read();
    grams = (raw - offset[2]) / scale_factor[2];
    Serial.print("Weight #3: ");
    Serial.print(grams);
    Serial.println(" g");
  }

  // Sensor 4
  // tcaselect(3);
  // if (nau4.available()) {
  //   raw = nau4.read();
  //   grams = (raw - offset[3]) / scale_factor[3];
  //   Serial.print("Weight #4: ");
  //   Serial.print(grams);
  //   Serial.println(" g");
  // }

  delay(1000);
}
