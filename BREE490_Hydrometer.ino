//V1.0 Soil_Humidity_Tester  Enjoy!
void setup() {
  // initialize  serial communication at 9600 bits per second:
  
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

}

//  the loop routine runs over and over again forever:
void loop() {
  // Set  LED colour to their Digital Ouputs.  Read the input on analog pin 0:
  int yellow  = 2;
  int blue = 3;
  int red = 4;
  int sensorValue = analogRead(A1);

  // Set the initial state of the LEDs to OFF
  digitalWrite(2, LOW);
  digitalWrite(3,  LOW);
  digitalWrite(4, LOW);
  
  // Logic Loop that sets the required  LED to ON
  if (sensorValue >= 1000) (digitalWrite(yellow, HIGH));
  else  if ((sensorValue <= 999) && (sensorValue >=901)) (digitalWrite(blue, HIGH));
  else if (sensorValue <= 900) (digitalWrite(red, HIGH));
  else ;

  //  Prints the condition of soil.  Dry, Wet or Perfect
  if (sensorValue >= 1000)  (Serial.print("SOIL IS TOO DRY!!!!!    "));
  else if ((sensorValue <= 999)  && (sensorValue >=901)) (Serial.print("SOIL IS PERFECT!!!!!    "));
  else  if (sensorValue <= 900) (Serial.print("SOIL IS TOO WET!!!!!    "));
  else;
  
  // print out the value you read:
  Serial.print("Marijuana Soil Humidity  is: ");
  Serial.println(sensorValue);
  delay(500);        // delay in between  reads for stability



}
