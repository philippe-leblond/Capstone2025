const int flowSensorPin = 2; // Pin where the flow sensor is connected
volatile int pulseCount = 0;
float flowRate = 0.0;
unsigned long oldTime = 0;

void setup() {
  pinMode(flowSensorPin, INPUT);
  digitalWrite(flowSensorPin, HIGH); // Optional: Enable internal pull-up
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), countPulse, RISING);
  Serial.begin(9600);
}

void loop() {
  if ((millis() - oldTime) > 1000) { // Only process once per second
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / 7.5; // Adjust the divisor based on your sensor's specifications
    oldTime = millis();
    pulseCount = 0;
    Serial.print("Flow rate: ");
    Serial.print(flowRate);
    Serial.println(" L/min");
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), countPulse, RISING);
  }
}

void countPulse() {
  pulseCount++;
}
