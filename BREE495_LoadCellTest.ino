#include <Adafruit_NAU7802.h>

Adafruit_NAU7802 nau;

long offset = 0;
float scale_factor = 1.0;

void setup() {
  Serial.begin(115200);
  if (!nau.begin()) {
    Serial.println("Failed to find NAU7802");
    while (1) delay(10);
  }
  Serial.println("Found NAU7802");

  // Let’s tare
  delay(1000);
  while (!nau.available()) delay(1);
  offset = nau.read();
  Serial.print("Offset (tare) = ");
  Serial.println(offset);

  // Put known weight (say 100 g) before continuing
  Serial.println("Place 496 g weight...");
  delay(5000); // give time to put the weight

  while (!nau.available()) delay(1);
  long raw_with_weight = nau.read();
  scale_factor = (float)(raw_with_weight - offset) / 496.0;
  Serial.print("Scale factor = ");
  Serial.println(scale_factor);
}

void loop() {
  while (!nau.available()) delay(1);
  long raw = nau.read();
  float grams = (raw - offset) / scale_factor;
  Serial.print("Weight: ");
  Serial.print(grams);
  Serial.println(" g");
}
