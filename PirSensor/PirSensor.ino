/*
  PIR Sensor

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/

const int pin_led = 9;                 // LED Out pin 
const int pin_pir = 2;                 // PIR In pin 
int pin_pir_stat = 0;                   // PIR status


// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(19200);

  pinMode(pin_pir, INPUT);
  pinMode(pin_led, OUTPUT);
 
  for(int i = 0; i < 20 ; i++){
   Serial.print(".");
   delay(1000);
  }

}

// the loop function runs over and over again forever
void loop() {

  pin_pir_stat = digitalRead(pin_pir); 

  Serial.println(pin_pir_stat);


  if (pin_pir_stat == HIGH)
  {
    Serial.println("Motion Detected");
    digitalWrite(pin_led, HIGH);
  } else {
    Serial.println("Motion Stopped");
    digitalWrite(pin_led, LOW);
  }

  delay(2000);
 
}
