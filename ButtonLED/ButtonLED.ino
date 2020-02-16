/*
  Button LED

  Turns an LED on via a push button.

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/

const int pin_led = 9;
const int pin_button = 8;
const int pin_pir = 10;                 // PIR Out pin 
int pin_pir_stat = 0;                   // PIR status


// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(19200);
  Serial.print(23);

  pinMode(pin_led, OUTPUT);
  pinMode(pin_button, INPUT_PULLUP);
  pinMode(pin_pir, INPUT);

}

// the loop function runs over and over again forever
void loop() {

  /*
  Serial.begin(9600);
  char buffer[16];
  sprintf(buffer,"pin button %d \n",digitalRead(pin_button));
  Serial.print(buffer);
  */

  /*
  if (digitalRead(pin_button) == LOW)
  {
    digitalWrite(pin_led, HIGH);
  } else {
    digitalWrite(pin_led, LOW);
  }
  */

  pin_pir_stat = digitalRead(pin_pir); 

  if (digitalRead(pin_pir_stat) == HIGH)
  {
    Serial.println("Motion Detected");
    digitalWrite(pin_led, HIGH);
  } else {
    Serial.println("Motion Stopped");
    digitalWrite(pin_led, LOW);
  }
 
}
