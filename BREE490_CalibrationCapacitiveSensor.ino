void setup() {
  Serial.begin(9600); // open serial port, set the baud rate to 9600 bps
}
void loop() {
  Serial.print("Sensor value: ");
  Serial.println(analogRead(35)); //connect sensor and print the value to serial
  Serial.print("voltage output: ");

  Serial.println(analogRead(35) *( 3.3 / 4095.0));

  delay(500);
}

