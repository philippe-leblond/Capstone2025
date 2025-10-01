// SPDX-FileCopyrightText: 2025 Adapted for NAU7802
//
// SPDX-License-Identifier: MIT

#include <Adafruit_NAU7802.h>

#define TCAADDR 0x70

// Create separate instances for each NAU7802
Adafruit_NAU7802 nau1;


// Helper function for changing TCA output channel
void tcaselect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();  
}

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println(F("One NAU7802 Example"));

  Wire.begin();

  // Initialize each NAU7802 on its TCA channel
  tcaselect(0);
  if (!nau1.begin()) {
    Serial.println("Failed to find NAU7802 #1");
  }
}

void loop() {
  int32_t val1, val2, val3, val4;

  tcaselect(0);
  val1 = nau1.read();

  Serial.println("------------------------------------");
  Serial.print("NAU7802 #1 = "); Serial.println(val1);

  delay(1000);

  
}
