/*
  PIR Sensor

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/

const int pin_led = 9;                 // LED Out pin 
const int pin_pir = 2;                 // PIR In pin 
volatile byte state = LOW;


// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(19200);

  pinMode(pin_pir, INPUT);
  pinMode(pin_led, OUTPUT);

  Serial.println("Sensor Calibration");

  for(int i = 0; i < 20 ; i++){
   Serial.print(".");
   delay(1000);
  }
  Serial.println("Sensor Calibrated");

  attachInterrupt(digitalPinToInterrupt(pin_pir),interrupt_routine,RISING);
}

// the loop function runs over and over again forever
void loop() {

  if (state == HIGH)
  {

    Serial.println("Motion Detected");
    Serial.println(digitalRead(pin_pir));

    digitalWrite(pin_led, HIGH);
    delay(5000);

    Serial.println("low");
    state = LOW;
    digitalWrite(pin_led,LOW);
  }
 
}

void interrupt_routine(){
  state = HIGH;
  Serial.println("interrupt");
}
