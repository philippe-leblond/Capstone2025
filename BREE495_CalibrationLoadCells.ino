#include <Adafruit_NAU7802.h>
#include <Preferences.h>

#define TCAADDR 0x70

// Create separate instances for each NAU7802
Adafruit_NAU7802 nau1;
Adafruit_NAU7802 nau2;
Adafruit_NAU7802 nau3;

// Store offset and scale factor for each load cell
long offset[3];
float scale_factor[3];

// Preferences object
Preferences prefs;

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

  // Save calibration to Preferences
  prefs.putLong(("offset" + String(sensorIndex)).c_str(), offset[sensorIndex]);
  prefs.putFloat(("scale"  + String(sensorIndex)).c_str(), scale_factor[sensorIndex]);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("Four NAU7802 with Calibration + Preferences"));

  Wire.begin();

  prefs.begin("loadcells", false); // namespace "loadcells"

  // Initialize each NAU7802
  tcaselect(0);
  nau1.begin();
  tcaselect(1);
  nau2.begin();
  tcaselect(2);
  nau3.begin();

  // Try loading calibration values
  bool calibrated = true;
  for (int i = 0; i < 3; i++) {
    offset[i]       = prefs.getLong(("offset" + String(i)).c_str(), 0);
    scale_factor[i] = prefs.getFloat(("scale" + String(i)).c_str(), 0);

    if (scale_factor[i] == 0) { // not calibrated
      calibrated = false;
    }
  }

  if (!calibrated) {
    Serial.println("No calibration found, starting calibration...");
    calibrateSensor(nau1, 0, 0, 592.0); // replace the value with the actual weight
    calibrateSensor(nau2, 1, 1, 612.5); // replace the value with the actual weight
    calibrateSensor(nau3, 2, 2, 681.0); // replace the value with the actual weight
    Serial.println("Calibration complete and saved!");
  } else {
    Serial.println("Calibration loaded from Preferences.");
  }
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

  delay(1000);
}
