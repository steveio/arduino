/*
  AnalogReadSerial

  Reads an analog input on pin 0, prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/AnalogReadSerial
*/

#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

byte note[8] = {
 B00010,
 B00011,
 B00010,
 B01110,
 B11110,
 B01100,
 B00000
};

byte smiley[8] = {
  B00000,
  B10001,
  B00000,
  B00000,
  B10001,
  B01110,
  B00000,
};

byte emptyHeart[8] = {
  B00000,
  B00000,
  B01010,
  B10101,
  B10001,
  B01010,
  B00100,
  B00000
};

byte fullHeart[8] = {
  B00000,
  B00000,
  B01010,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
};

byte bell[8] = {
  B00100,
  B01110,
  B01110,
  B01110,
  B11111,
  B00000,
  B00100
};

// the setup routine runs once when you press reset:
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(19200);
  delay(500);//Delay to let system boot

  lcd.createChar(0, smiley);
  lcd.createChar(1, emptyHeart);
  lcd.createChar(2, fullHeart);
  lcd.createChar(3, note);
  lcd.createChar(4, bell);
 
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
 
  lcd.clear();
}

// the loop routine runs over and over again forever:
void loop() {

  lcd.setCursor(0, 1);
 
  //Display a message with the custom characters
  lcd.write(byte(4)); //display custom character related associated with num 4
  lcd.print(" Alarm Set! ");
  lcd.write(byte(3)); //display custom character related associated with num 3
  lcd.cursor();
  lcd.blink();
  delay(6000);
  lcd.clear();

  // switch off display
  lcd.noCursor();
  lcd.noBlink();
 
  lcd.write("Hello World!");
  delay(3000);
  for(int i=0; i<16; i++)
  {
    lcd.scrollDisplayLeft();
    delay(500);
  }

  for(int i=0; i<16; i++)
  {
    lcd.scrollDisplayRight();
    delay(500);
  }

  lcd.noDisplay();
  delay(2000);
  lcd.display();
  lcd.clear();

}
