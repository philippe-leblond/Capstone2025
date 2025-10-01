#include <Preferences.h>

Preferences prefs;

int counter = 0;

void setup() {
  Serial.begin(115200);

  prefs.begin("my-app", false);

  // Load saved value
  counter = prefs.getInt("counter", 0);

  prefs.end();
}

void loop() {
  Serial.print("Counter: ");
  Serial.println(counter);

  counter++;

  prefs.begin("my-app", false);
  prefs.putInt("counter", counter);
  prefs.end();

  delay(1000);
}
