/**
  Weather Station

  Example sketch for an Arduino weather module built with an ATmega2560 microcontroller

  Wake on a configurable schedule (every minute) to read sensor sample data

  Hardware includes:
    DS3231 RTC
    DHT11 Temperature & Humidity
    BMP180 Barometric Pressure
    Water Sensor
    SD Card Module
    LDR PhotoResistor 
    6v 2.0watt 333mA Solar Panel
    TP4056 Battery Charge Controller
    2x Li-ion 3.7v 3600mAh Battery

  Requires:
    https://github.com/steveio/AlarmSchedule
    https://github.com/steveio/RTCLib
  
  This example code is in the public domain.
*/

#include <Wire.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <DateTime.h>
#include <DHT.h>
#include <LiquidCrystal.h>
#include <Adafruit_BMP085.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>

#include <AlarmSchedule.h>
#include <avr/sleep.h>
#include <avr/power.h>

const long baud_rate = 115200;

// Mega2560 RX1/TX1
byte rx = 19;
byte tx = 18;


// RTC DS3231
DS3232RTC rtc;
#define RTC_SDA_PIN 20
#define RTC_SCL_PIN 21
#define RTC_INTERRUPT_PIN 2

volatile int8_t interuptState = 0;

AlarmScheduleRTC3232 als;

DateTime dt;
uint32_t ts;

// SD Card - Data Logger
File logFile;
char logFileName[] = "00000000.txt";
const int chipSelect = 53;

// DHT11 Temperature / Humidity
// Wiring (S)ignal | VCC | GND
#define dht_apin A0
DHT dht(dht_apin, DHT11);

// BMP180 Barometric Pressure
Adafruit_BMP085 bmp;
float bmpPressure = 0;

// LDR - Light Dependant Resistor (S)A0, VCC, -GND, 
int ldr_apin = A1;
int ldr_apin_val = 0;
const int ldr_daynight_threshold = 300; // day/night level, 300 in range of direct sunlight
int ldr_adjusted;

// water / moisture sensor
int wSensorPin = A2;
int wSensorVal = 0;

// DHT11 humidity, temp c, temp f
float h, tc, tf; 
int h1;


boolean lcd_active = 0;
boolean sd_active = 1; // enable/disable SD Card Data Logging
boolean wifi_tx_active = 1; // transmit JSON over software serial to ESP8266

/**
 * Schedule RTC Alarm every minute at :00 secs
 */
void setAlarm()
{
  Serial.println("setAlarm");
  als.setAlarm( ALM1_MATCH_SECONDS, 0, 0, 0, 0, 0);
}


/**
 * Get dated log filename in format YYYYMMDD.txt
 */
void getLogFileName()
{
  sprintf(logFileName, "%02d%02d%02d.txt", dt.year(), dt.month(), dt.day());
}


/**
 * Read Samples from Sensor Array
 */
void sensorRead()
{
  readWater();
  readLDR();
  readDHT11();
  readBMP180();
}


void readWater()
{
  wSensorVal = analogRead(wSensorPin);
  Serial.println("Water Sensor: ");
  Serial.println(wSensorVal, DEC);
}

void readLDR()
{
  ldr_apin_val = analogRead(ldr_apin);
  Serial.println("Light Dependant Resistor (LDR): ");
  Serial.println(ldr_apin_val, DEC);
}

void readDHT11()
{

  Serial.println("DHT11: Temp, Humidity ");

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  tc = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  tf = dht.readTemperature(true);

  if (isnan(h) || isnan(tc)) {
    Serial.println("DHT11 Sensor ERROR");
  }

  h1 = floor(h*1000)/1000; 

  Serial.println("Humidity:");
  Serial.println(h1);
  Serial.println("Temperature (celsuis):");
  Serial.println(tc,1);
  Serial.println("Temperature (fahrenheit):");
  Serial.println(tf);
  
}

void readBMP180()
{

    Serial.println("BMP180: Air Pressure ");

    Serial.print("Temperature = ");
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");

    bmpPressure = bmp.readPressure(); 
    Serial.print("Pressure = ");
    Serial.print(bmpPressure);
    Serial.print(" Pa");

    float hiThreshold = 102268.9; // Pa / 30.20 inHg
    float lowThreshold = 100914.4; // Pa / 30.20 inHg

    if (bmp.readPressure() > hiThreshold)
    {
      Serial.println(" (High)");
    } else {
      Serial.println(" (Low)");
    }
    
    // Calculate altitude assuming 'standard' barometric
    // pressure of 1013.25 millibar = 101325 Pascal
    Serial.print("Altitude = ");
    Serial.print(bmp.readAltitude());
    Serial.println(" meters");

    Serial.print("Pressure at sealevel (calculated) = ");
    Serial.print(bmp.readSealevelPressure());
    Serial.println(" Pa");

    Serial.print("Real altitude = ");
    Serial.print(bmp.readAltitude(101500));
    Serial.println(" meters");
    
    Serial.println();

}

void setup() {

  Serial.begin(baud_rate);
  Serial1.begin(baud_rate);

  pinMode(rx,INPUT);
  pinMode(tx,OUTPUT);

  delay(1000);

  Serial.println("Weather Station: Begin\n");
  Serial1.println("Arduino Mega2560 Weather Station Begin!");

  
  pinMode(ldr_apin,INPUT);
  pinMode(ldr_apin,INPUT);

  dht.begin();

  bmp.begin();

  Serial.println("RTCDS3231 Begin!");

  pinMode(RTC_SDA_PIN, INPUT);
  pinMode(RTC_SCL_PIN, INPUT);
  rtc.begin();

  DateTime rtc_t = rtc.get();

  Serial.println(__DATE__);
  Serial.println(__TIME__);

  DateTime arduino_t = DateTime(__DATE__, __TIME__);
  setInternalClock();
  setRTC(); // sync RTC clock w/ system time

  pinMode( RTC_INTERRUPT_PIN, INPUT_PULLUP);
  setAlarm();
  sleep();

}


void loop() {

  dt = rtc.get();
  ts = dt.unixtime(); // unix ts for logging

  char dtm[32];
  sprintf(dtm, "%02d/%02d/%02d %02d:%02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute(),dt.second());
  Serial.println(dtm);

  if (interuptState == 1) // ISR called
  {
    interuptState = 0;

    sensorRead();
    serialiseJSON();

    setAlarm();
    sleep();
  }

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
  
    if (ldr_adjusted < ldr_daynight_threshold)
    {
      lcd.print(F("Day"));
    } else {
      lcd.print(F("Night")); 
    }
    */

}

/**
 * Serialise a single data sample formatted as JSON object
 * If SD Card Module is enabled write data to file
 * 
 */
void serialiseJSON()
{

  const size_t capacity = JSON_OBJECT_SIZE(12);
  
  DynamicJsonDocument doc(capacity);
  
  
  JsonObject data = doc.createNestedObject();
  data["ts"] = ts;
  data["tempC"] = tc;
  data["tempF"] = tf;
  data["h"] = h;
  data["LDR"] = ldr_apin_val;
  data["p"] = bmpPressure;
  data["w"] = wSensorVal;


  char json_string[256];
  serializeJson(doc, json_string);

  if (sd_active) {
    SDCardWrite(json_string);
  }

  if (wifi_tx_active) {
    Serial1.println(json_string);
    Serial1.println("\n");
    Serial1.flush();
  }

}


void SDCardWrite(char * json_string)
{

  if (!SD.begin()) {
    Serial.println("SD init failed!");
    return;
  }

  getLogFileName();
  //Serial.print("SD Log File: ");
  //Serial.println(logFileName);

  logFile = SD.open(logFileName, FILE_WRITE);

  if (logFile) {

    Serial.println(json_string);
    logFile.println(json_string);
    logFile.close();

    Serial.println("SD write OK.");

  } else {
    Serial.println("SD write error");
  }

}

/**
 * Set RTC clock based on system time
 */
void setRTC()
{

  DateTime rtc_t = rtc.get();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  if (rtc_t.unixtime() < arduino_t.unixtime()) 
  {
    Serial.println("RTC set()");
    rtc.set(arduino_t.unixtime());    

    Serial.println("RTC unixtime()");
    Serial.println(rtc_t.unixtime());

  }
}

/**
 * Set Arduino Internal Clock (sysTime) to compile time 
 */
void setInternalClock()
{
  DateTime compile_t = DateTime(__DATE__, __TIME__);
  setTime(compile_t.unixtime());
  Serial.println(compile_t.unixtime());
}

/*
 * Maintain Arduino system time based on RTC
 */
void setSyncProviderRTC()
{
    setSyncProvider( syncProviderRTC ); // identify the external time provider
    time_t sync_interval = 60*60*24; // once a day
    setSyncInterval(sync_interval); // set the number of seconds between re-sync

}

/**
 * Return ts of last internal clock sync
 */
timeStatus_t getLastClockSync()
{
    timeStatus_t last_sync = timeStatus(); // last clock sync
    Serial.println("Last clock sync: ");
    Serial.println(last_sync);
    return last_sync;
}

/*
 * Return current time_t as input for setSyncProvider( syncProviderRTC );
 */
time_t syncProviderRTC()
{
  DateTime rtc_ts = rtc.get();
  return rtc_ts.unixtime();  
}

void sleep()
{
  Serial.println("sleep()");
  delay(1000);

  for (int i = 0; i <= 53; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  digitalWrite(13, LOW); // turn off internal LED


  power_adc_disable();
  power_spi_disable();
  power_usart0_disable();
  power_usart2_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_timer3_disable();
  power_timer4_disable();
  power_timer5_disable();
  power_twi_disable();

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  pinMode(RTC_INTERRUPT_PIN, INPUT_PULLUP);
  digitalWrite(RTC_INTERRUPT_PIN, HIGH);
  attachInterrupt(0, wakeUp, LOW);
  delay(500);

  sleep_mode();
}

void wakeUp()
{
  sleep_disable();
  detachInterrupt(0);

  power_adc_enable();
  power_spi_enable();
  power_usart0_enable();
  power_usart2_enable();
  power_timer1_enable();
  power_timer2_enable();
  power_timer3_enable();
  power_timer4_enable();
  power_timer5_enable();
  power_twi_enable();

  interuptState = 1;

  digitalWrite(13, HIGH); // turn on internal LED

  Serial1.println(" \n");
}
