/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/

const int red_led_pin = 9;
const int button_pin = 8;

//int green_led_pin = 10;
//int blue_led_pin = 11;


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(red_led_pin, OUTPUT);
  //pinMode(green_led_pin, OUTPUT);
  //pinMode(blue_led_pin, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {


  RGB_color(255,0,0);
  delay(1000);

  /*
  RGB_color(0,255,0);
  delay(1000);
  RGB_color(0,0,255);
  delay(1000);
  */
}

void RGB_color(int red_led_value, int green_led_value, int blue_led_value)
{
  Serial.begin(9600);
  char buffer[16];
 
  sprintf (buffer, "RED_LED: %d \n",red_led_value);
  Serial.print(buffer);
  //analogWrite(red_led_pin, red_led_value);
  digitalWrite(red_led_pin, HIGH);

  /*
  sprintf (buffer, "GREEN_LED: %d \n",red_led_value);
  Serial.print(buffer);
  analogWrite(green_led_pin, green_led_value);

  sprintf (buffer, "BLUE_LED: %d \n",blue_led_value);
  Serial.print(buffer);
  analogWrite(blue_led_pin, blue_led_value);
  */
}
