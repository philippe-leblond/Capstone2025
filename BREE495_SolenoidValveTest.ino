const int solenoidValve = 2;
bool solenoidState = false;

void controlSolenoid(float avgMoisture) {
  if (avgMoisture < 30.0) {
    solenoidState = true;
  } else if (avgMoisture > 50.0) {
    solenoidState = false;
  }
  digitalWrite(solenoidValve, solenoidState ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(solenoidValve, OUTPUT);
}

void loop() {
  static float testMoisture = 25.0;  // You can change this value to simulate conditions

  controlSolenoid(testMoisture);
  Serial.print("Moisture: ");
  Serial.print(testMoisture);
  Serial.print(" %  |  Solenoid State: ");
  Serial.println(solenoidState ? "ON" : "OFF");

  delay(2000);

  // Simulate moisture increasing
  testMoisture += 5;
  if (testMoisture > 60) testMoisture = 25;
}
