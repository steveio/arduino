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


#include "Relay.h"

RTC_DS1307 rtc;
DateTime dt;


unsigned long startTime = 0;
unsigned long sampleInterval = 10000;  // sample frequency in ms
unsigned long sampleTimer = 0;

#define GPIO_VALVE1 3
#define SZ_TIME_ELEMENT 3

Timer valve1Timer;
// create variables to define on/off time pairs 
struct tmElements_t t1_on, t1_off, t2_on, t2_off, t3_on, t3_off;
struct tmElementArray_t timeArray;


Relay valve1(GPIO_VALVE1);


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

  timeArray.n = 2;
  timeArray.Wday = 0b01111111; // define which days of week timer is active on

  timeArray.onTime[0] = t1_on;
  timeArray.offTime[0] = t1_off;
  timeArray.onTime[1] = t2_on;
  timeArray.offTime[1] = t2_off;

  valve1Timer.init(TIMER_MINUTE, &timeArray);
  valve1.initTimer(valve1Timer);

}

void loop() {

  if (millis() >= sampleTimer + sampleInterval)
  {

    char dtm[32];
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
    Serial.println(dtm);
    
    dt = rtc.now();
    
    if (valve1.timer.isScheduled(dt.minute(), dt.hour(), dt.dayOfTheWeek()))
    {
      Serial.println("Valve: on");
      valve1.on();
    } else {
      Serial.println("Valve: off");
      valve1.off();
    }
  }

}
