/*
  AnalogReadSerial

  Reads an analog input on pin 0, prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/AnalogReadSerial
*/

#include <DHT.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"

DS1307 rtc;

#define dht_apin A0
DHT dht(dht_apin, DHT11);

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);


void setup() {
  Serial.begin(19200);

  rtc.begin();

#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif

  Serial.println("RTC adjust()");
  rtc.adjust(DateTime(__DATE__, __TIME__));
 

  Serial.println("DHT11 Humidity & temperature Sensor\n\n");
  delay(1000);//Wait before accessing Sensor
  dht.begin();

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  lcd.print("Initializing");
  for(int i=0; i<3; i++)
  {
    lcd.print(".");
    delay(1000);
  }
}

// the loop routine runs over and over again forever:
void loop() {

  lcd.clear();
  lcd.setCursor(0, 0);

  DateTime now = rtc.now();

  char dt[16];
  char tm[16];
  
  sprintf(dt, "%02d/%02d/%02d", now.day(),now.month(),now.year());
  sprintf(tm, "%02d:%02d", now.hour(),now.minute());

  lcd.print(dt);
  lcd.setCursor(0, 1);
  lcd.print(tm);

  delay(5000);

  lcd.clear();
  lcd.print("Temp:  Humidity:");

  lcd.setCursor(0, 1);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(f)) {
    lcd.print("Sensor ERROR");
    return;
  }

  int h1 = floor(h*1000)/1000; 

  /**
   * Temperature Rounding Methods 
   *  
  Serial.write(f,2);
  lcd.write(f,2); // Implements Print::
  
  // replace 100 with pow(100, (int)desiredPrecision)
  floor(19.12345 × 100) ÷ 100.0
  round(19.12345 × 100) ÷ 100.0
  ceil(19.12345 × 100) ÷ 100.0
  
  printf("%.2f", 37.777779);
  sprintf(s,"%.2f",t)

   */


  lcd.print(t,2);
  /** 
   *  @see https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
   *  Table 4.  Character Codes / Patterns (ROM Code)
   *  0xdf - B1101 1111
   **/
  lcd.print((char)223);
  lcd.setCursor(7,1);
  lcd.print(h1);
  lcd.print("%");

  Serial.println("DHT11 Humidity:");
  Serial.println(h);
  Serial.println("DHT11 Temperature (celsuis):");
  Serial.println(t,1);
  Serial.println("DHT11 Temperature (fahrenheit):");
  Serial.println(f);

  // Wait a few seconds between measurements.
  delay(10000);

}
