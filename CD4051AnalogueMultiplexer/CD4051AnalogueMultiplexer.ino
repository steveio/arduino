const int selectPins[3] = {2, 3, 4}; // S0~2, S1~3, S2~4
const int zInput = A0; // Connect common (Z) to A0 (analog input)

void setup() 
{
  Serial.begin(115200); // Initialize the serial port
  // Set up the select pins as outputs:
  for (int i=0; i<3; i++)
  {
    pinMode(selectPins[i], OUTPUT);
    digitalWrite(selectPins[i], HIGH);
  }
  
  // Print the header:
  Serial.println("Y0\tY1\tY2\tY3\tY4\tY5\tY6\tY7");
  Serial.println("---\t---\t---\t---\t---\t---\t---\t---");
}

void loop() 
{
  // Loop through all eight pins.
  for (byte pin=0; pin<=7; pin++)
  {
      for (int i=0; i<3; i++) {
          digitalWrite(selectPins[i], pin & (1<<i)?HIGH:LOW);
      }
      int inputValue = analogRead(zInput);
      Serial.print(String(inputValue) + "\t");
  }
  Serial.println();
  delay(1000);
}
