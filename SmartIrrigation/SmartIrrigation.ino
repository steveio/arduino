/**
 * Smart Irrigation
 * 
 * Programable garden / greenhouse / cold frame watering controller
 * 
 *  -- 2x Solenoid valves switched via TiP120 darlington transistor
 *  -- TimedDevice library for scheduling
 *  -- DHT22 for temperature adjusted watering cycle
 * 
 * Hardware:
 * Arduino Nano
 * DS3231 RTC 
 * 2x Solenoid Valve
 * DHT22 Temperature Sensor
 *
 * @requires Library TimedDevice ( https://github.com/steveio/TimedDevice )
 *
 *
 */

#include <Wire.h>
#include "RTClib.h"
#include <DHT.h>;
#include "Relay.h"

RTC_DS1307 rtc;
DateTime dt;

unsigned long startTime = 0;
unsigned long sampleInterval = 30000;  // sample frequency in ms
unsigned long sampleTimer = 0;

#define GPIO_VALVE1 3
#define SZ_TIME_ELEMENT 3

Timer timer1Cycle, timer2Cycle;
// create variables to define on/off time pairs 
struct tmElements_t t1_on, t1_off, t2_on, t2_off, t3_on, t3_off;
struct tmElementArray_t timeArray1Cycle, timeArray2Cycle;


Relay valve1(GPIO_VALVE1);

// DHT22 Temperature Sensor
#define DHTPIN 4
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);

float h;     // Humidity (%)
float tempC; // Temperature (celcius)
float tempHiThreshold = 18; // defines a "hot" day

// Daily Hi/Low Temperature / Humidity
int currDay = dt.day();

// Hi / Low - Indexed by day of week (0 = Sun - 6 = Sat)
float hiT[7] = { 0 }; // T = Temp
float loT[7] = { 0 };
float hiH[7] = { 0 }; // H = Humidity
float loH[7] = { 0 };


void setup() {

  Serial.begin(115200);

  startTime = millis();

  Wire.begin();
  Wire.setClock(400000L);

  rtc.begin();
  dt = rtc.now();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  // sync RTC w/ arduino time @ compile time
  if (dt.unixtime() != arduino_t.unixtime())
  {
    rtc.adjust(arduino_t.unixtime());
  }

  dht.begin();

  pinMode(GPIO_VALVE1, OUTPUT);

  // Solenoid Valve On/Off times and DayOfWeek
  t1_on.Hour = 8;
  t1_on.Min = 0;
  t1_off.Hour = 8;
  t1_off.Min = 10;

  t2_on.Hour = 19;
  t2_on.Min = 0;
  t2_off.Hour = 19;
  t2_off.Min = 10;

  timeArray1Cycle.n = 1;
  timeArray1Cycle.Wday = 0b01111111; // define which days of week timer is active on

  timeArray1Cycle.onTime[0] = t1_on;
  timeArray1Cycle.offTime[0] = t1_off;

  timer1Cycle.init(TIMER_MINUTE, &timeArray1Cycle);


  timeArray2Cycle.n = 2;
  timeArray2Cycle.Wday = 0b01111111; // define which days of week timer is active on

  timeArray2Cycle.onTime[0] = t1_on;
  timeArray2Cycle.offTime[0] = t1_off;
  timeArray2Cycle.onTime[1] = t2_on;
  timeArray2Cycle.offTime[1] = t2_off;

  timer2Cycle.init(TIMER_MINUTE, &timeArray2Cycle);

  // default to 1 morning watering cycle
  valve1.initTimer(timer1Cycle);

  sampleTimer = millis();
}

void loop() {

  if (millis() >= sampleTimer + sampleInterval)
  {
    char dtm[32];
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
    Serial.println("");
    Serial.println(dtm);
    
    dt = rtc.now();
    
    if (valve1.timer.isScheduled(dt.minute(), dt.hour(), dt.dayOfTheWeek()))
    {
      Serial.println(F("Valve:\ton"));
      valve1.on();
    } else {
      Serial.println(F("Valve:\toff"));
      valve1.off();
    }

    valve1.timer.printSchedule(Serial);
    updateStats();
    printStats();
    setWaterCycleFreq();

    sampleTimer = millis();

  }
}

// On "hot" days add 2nd evening watering cycle
void setWaterCycleFreq()
{
  if (hiT[dt.dayOfTheWeek()] > tempHiThreshold)
  {
    valve1.initTimer(timer2Cycle);
  } else {
    valve1.initTimer(timer1Cycle);
  }
}
  

// Track daily hi/low temperature / humdity
void updateStats()
{
  h = dht.readHumidity();
  tempC = dht.readTemperature();

  if (NULL == hiT[dt.dayOfTheWeek()] || tempC > hiT[dt.dayOfTheWeek()])
  {
    hiT[dt.dayOfTheWeek()] = tempC;
  }
  if (NULL == loT[dt.dayOfTheWeek()] || tempC < loT[dt.dayOfTheWeek()])
  {
    loT[dt.dayOfTheWeek()] = tempC;
  }

  if (NULL == hiH[dt.dayOfTheWeek()] || h > hiH[dt.dayOfTheWeek()])
  {
    hiH[dt.dayOfTheWeek()] = h;
  }
  if (NULL == loH[dt.dayOfTheWeek()] || h < loH[dt.dayOfTheWeek()])
  {
    loH[dt.dayOfTheWeek()] = h;
  }

  if (currDay != dt.dayOfTheWeek())
  {
    currDay = dt.dayOfTheWeek();
    hiT[dt.dayOfTheWeek()] = 0;
    loT[dt.dayOfTheWeek()] = 0;
    hiH[dt.dayOfTheWeek()] = 0;
    loH[dt.dayOfTheWeek()] = 0;
  }
}

void printStats()
{  
    Serial.print(F("TempC:\t"));
    Serial.print(tempC);
    Serial.print(F(" ( "));
    Serial.print(loT[dt.dayOfTheWeek()]);
    Serial.print(F(" / "));
    Serial.print(hiT[dt.dayOfTheWeek()]);
    Serial.println(F(" ) "));
    Serial.print(F("Humid:\t"));
    Serial.print(h);
    Serial.print(F(" ( "));
    Serial.print(loH[dt.dayOfTheWeek()]);
    Serial.print(F(" / "));
    Serial.print(hiH[dt.dayOfTheWeek()]);
    Serial.println(F(" ) "));

}
