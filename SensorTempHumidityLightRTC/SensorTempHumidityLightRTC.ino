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
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>


File myFile;
const int chipSelect = 53;

const char app_name[12] = "sensor.meteo";
const char client_key[128] = "128BITHASHKEY";

DS1307 rtc;

int sensorOnMins = 10;

float h, tc, tf; // humidity, temp c, temp f
int h1; 

#define dht_apin A0
DHT dht(dht_apin, DHT11);


// LRD - Light Resistant Diode
int prs_apin = A1;
int prs_apin_val = 0;
const int prs_threshold = 300; // day/night level, 300 in range of direct sunlight
int prs_adjusted;


int led_apin = 9;
int led_brightness;

boolean lcd_active = 1;
boolean led_active = 0;
boolean sd_active = 1;


void setup() {

  Serial.begin(115200);

  delay(1000);

  rtc.begin();
  Wire.begin();
  dht.begin();

  Serial.println("RTC adjust()");
  rtc.adjust(DateTime(__DATE__, __TIME__));

  pinMode(prs_apin,INPUT);
  pinMode(led_apin,OUTPUT);

  Serial.println("Temperature, Humidity & Light Sensor\n");

}


void loop() {

  DateTime now = rtc.now();

  DateTime compiled = DateTime(__DATE__, __TIME__);
  if (now.unixtime() < compiled.unixtime()) 
  {
    Serial.println("RTC adjust");
    rtc.adjust(DateTime(__DATE__, __TIME__));    
  }

  char dt[16];
  char tm[16];
  char dtm[32];
  
  sprintf(dt, "%02d/%02d/%02d", now.day(),now.month(),now.year());
  sprintf(tm, "%02d:%02d", now.hour(),now.minute());
  sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , now.day(),now.month(),now.year(),now.hour(),now.minute());

  // unix timestamp, useful for logging and date/time calculation
  uint32_t ts =now.unixtime();

  Serial.println(dtm);
  //Serial.println(ts);

  //if (now.minute() == 38)
  if (now.minute() == 00 || now.minute() == 15 || now.minute() == 30 || now.minute() == 45)
  {
    Serial.println("Alarm - Read Sensor:");

    sensorRead();
  
    if (led_active) {
      led_brightness = map(prs_adjusted, 0, 1023, 0, 255);
      analogWrite(led_apin, led_brightness);
    }
  

    if (sd_active) {
      SDCardWrite(ts, tc, tf, h, prs_apin_val);
    }

    //serialiseJSON(ts, tc, tf, h, prs_apin_val);

    if (lcd_active) {
      LCDWrite(dt,tm,tc,h1,prs_apin_val);
    }

    Serial.println("");
    delay(60000);
  }


  delay(30000);
}

void sensorRead()
{

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  tc = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  tf = dht.readTemperature(true);

  if (isnan(h) || isnan(tc)) {
    Serial.println("Sensor ERROR");
    return;
  }

  h1 = floor(h*1000)/1000; 

  /**
   * Temperature Rounding Methods 
   *   
  // replace 100 with pow(100, (int)desiredPrecision)
  floor(19.12345 × 100) ÷ 100.0
  round(19.12345 × 100) ÷ 100.0
  ceil(19.12345 × 100) ÷ 100.0
  // ! (s)printf() support for %f float format
  printf("%.2f", 37.777779);
  sprintf(s,"%.2f",t)
  */

  Serial.println("DHT11 Humidity:");
  Serial.println(h);
  Serial.println("DHT11 Temperature (celsuis):");
  Serial.println(tc,1);
  Serial.println("DHT11 Temperature (fahrenheit):");
  Serial.println(tf);

  prs_apin_val = analogRead(prs_apin);
  Serial.println("PhotoResistor: ");
  Serial.println(prs_apin_val, DEC);

  // shift linear scale
  int shift_pct = 75;
  // led brightness inversely proportionate to LRD light level + shift factor
  prs_adjusted = prs_apin_val - (prs_apin_val * shift_pct / 100);
  // led brightness proportionate to LRD light level
  prs_adjusted = 1023 - prs_apin_val;
  //scale: map 0-1023 to 0-255 (range analogWrite)

}

void LCDWrite(char *dt, char *tm,float tc, int h1, int lrd)
{

    LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

    lcd.begin(16, 2);

    /*
    lcd.print("Initializing");
    for(int i=0; i<3; i++)
    {
      lcd.print(".");
      delay(1000);
    }
    */

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dt);
    lcd.print(" ");
    lcd.print(tm);

    lcd.setCursor(0, 1);

    lcd.print(tc,2);
    lcd.print((char)223);
    lcd.setCursor(7,1);
    lcd.print(h1);
    lcd.print("%");
    lcd.print(" ");
    lcd.print(lrd,DEC);

    /*
    delay(5000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PhotoResistor: ");
    lcd.setCursor(0, 1);
    lcd.print(lrd,DEC);
    lcd.print(" : ");
  
    if (prs_adjusted < prs_threshold)
    {
      lcd.print(F("Day"));
    } else {
      lcd.print(F("Night")); 
    }
    */

}

/**
 * Serialise a single data sample formatted as JSON object
 * When is JSON too verbose for sensor data? Is structure & readability (of keys) worth payload duplication?
 * Is a simple delimited data format better suited for sensor LAN ?
 * 
 * @param int unix timestamp
 * @param float temp (celsuis)
 * @param float temp (fahrenheit)
 * @param float humidity
 * @param int LDR sensor value
 */
void serialiseJSON(uint32_t ts, float tc, float tf, float h, int ldr)
{

  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5);
  
  DynamicJsonDocument doc(capacity);
  
  doc["app"] = app_name;
  doc["api_key"] = client_key;
  
  JsonArray data = doc.createNestedArray("data");
  
  JsonObject data_0 = data.createNestedObject();
  data_0["ts"] = ts;
  data_0["tempC"] = tc;
  data_0["tempF"] = tf;
  data_0["humidity"] = h;
  data_0["LDR"] = ldr;

  // Parse from Serial Log (header prob not required)
  // grep "^{" sensor.meteo.json.raw  | awk '{ print substr($1,1,length($1)-2)"," }' | awk '{ print substr($1,31,length($1))}'
  serializeJson(doc, Serial);

  // Start a new line
  Serial.println();

  // Generate the prettified JSON and send it to the Serial port.
  // serializeJsonPretty(doc, Serial);

}

void SDCardWrite(uint32_t ts, float tc, float tf, float h, int ldr)
{

  if (!SD.begin()) {
    Serial.println("SD init failed!");
    return;
  }

  myFile = SD.open("sensor.txt", FILE_WRITE);

  if (myFile) {

    char s1[8], s2[8], s3[8];
    floatToString(tc,s1);
    floatToString(tf,s2);
    floatToString(h,s3);

    /*
    char b[128];
    sprintf(b,"%s", s1);
    Serial.println(b);
    */

    char b[128];
    sprintf(b,"%lu,%s,%s,%s,%d", ts, s1, s2, s3, ldr);

    Serial.println(b);
    myFile.println(b);

    /* Or print each component seperately...
    myFile.print(ts);
    myFile.print(",");
    myFile.print(tc);
    myFile.print(",");
    myFile.print(tf);
    myFile.print(",");
    myFile.print(h);
    myFile.print(",");
    myFile.print(ldr);
    myFile.print("\n");
    */
    
    myFile.close();
    Serial.println("SD write OK.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("SD error: sensor.txt");
  }


}

void floatToString(float f, char * str)
{ 
  char *tmpSign = (f < 0) ? "-" : "";
  float tmpVal = (f < 0) ? -f : f;
  
  int tmpInt1 = tmpVal;                  // Get the int (XXX.)
  float tmpFrac = tmpVal - tmpInt1;      // Get fraction (0.0XXX).
  int tmpInt2 = ceil(tmpFrac * 100);
  //int tmpInt2 = trunc(tmpFrac * 10000);
 
  sprintf (str, "%s%d.%02d", tmpSign, tmpInt1, tmpInt2);
}
