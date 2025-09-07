const int sensorPin = 15; // ADC pin (e.g., GPIO34)
const int numReadings = 10; // Number of readings to average

const int dryValue = 3700;  // Calibration: sensor in dry air measured with the code capacitive sensor calibration
const int wetValue = 570;  // Calibration: sensor in saturated soil measured with the code capacitive sensor calibration

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for Serial Monitor
}

void loop() {
  long total = 0;

  // Take multiple readings for averaging
  for (int i = 0; i < numReadings; i++) {
    total += analogRead(sensorPin);
    delay(20); // Small delay between readings
  }

  int averageValue = total / numReadings;

  // Convert to moisture percentage (invert: lower = wetter)
  float moisturePercent = map(averageValue, dryValue, wetValue, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100); // Clamp 0–100%

  // Optional: convert to voltage
  float voltage = averageValue * (3.3 / 4095.0);

  // Print output
  Serial.print("Avg ADC: ");
  Serial.print(averageValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);
  Serial.print(" V | Moisture: ");
  Serial.print(moisturePercent);
  Serial.println(" %");

  delay(1000); // Main loop delay
}
