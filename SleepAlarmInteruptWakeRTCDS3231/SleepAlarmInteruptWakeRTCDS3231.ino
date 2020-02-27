/** 
 * Arduino RTC DS3231/3232
 * 
 * Schedule an interupt alarm to wake Arduino at intervals
 * Implement low power saving mode and sleep between tasks
 * Optimise for long duration battery operation mode
 * 
 * RTCDS3231 DataSheet: https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
 * 
 * @requires DateTime.h from: https://github.com/steveio/RTCLib
 * 
 */

#include <Wire.h>
#include <DS3232RTC.h>  
#include <Time.h>
#include <DateTime.h>

#include <avr/sleep.h>
#include <avr/power.h>

DS3232RTC rtc;

#define RTC_SDA_PIN 18                         
#define RTC_SCL_PIN 19  

int rtcInterruptPin = 2;
void (*isrInteruptFunc)() = &rtcInterupt;  // RTC interupt function

volatile int8_t deviceState = 0; // 0 = sleep, 1 = wakeup
volatile int8_t interuptState = 0;

volatile unsigned long lastInterruptMillis;
volatile unsigned long currentMillis;
const unsigned long interruptWaitPeriod = 1000;

/*
 * Alarm Schedule Constants:
 *
ALM1_EVERY_SECOND = 0x0F,      // once a second
ALM1_MATCH_SECONDS = 0x0E,     // when seconds match
ALM1_MATCH_MINUTES = 0x0C,     // match minutes *and* seconds
ALM1_MATCH_HOURS = 0x08,       // match hours *and* minutes, seconds
ALM1_MATCH_DATE = 0x00,        // match date *and* hours, minutes, seconds
ALM1_MATCH_DAY = 0x10,         // match day *and* hours, minutes, seconds
ALM2_EVERY_MINUTE = 0x8E,
ALM2_MATCH_MINUTES = 0x8C,     // match minutes
ALM2_MATCH_HOURS = 0x88,       // match hours *and* minutes
ALM2_MATCH_DATE = 0x80,        // match date *and* hours, minutes
ALM2_MATCH_DAY = 0x90,         // match day *and* hours, minutes
*/
ALARM_TYPES_t alarmType = ALM1_MATCH_SECONDS;

// Set Alarm at specific times
uint8_t alarmMatchSec = 0;  // 00-59
uint8_t alarmMatchMin = 0;  // 00-59
uint8_t alarmMatchHour = 0; // 00-23
uint8_t alarmMatchDay = 0;  // 1-7

// Or set repeat Alarm (ALM1_MATCH_SECONDS | ALM1_MATCH_MINUTES) in (secs|mins)
uint8_t alarmInterval = 30; 


void setup () {

  Serial.begin(115200);
  Serial.println("RTCDS3231 Begin!");

  pinMode(RTC_SDA_PIN, INPUT);
  pinMode(RTC_SCL_PIN, INPUT);

  pinMode(13,OUTPUT);
  digitalWrite(13, HIGH); // turn off internal LED

  rtc.begin();

  Serial.println(__DATE__);
  Serial.println(__TIME__);

  DateTime rtc_t = rtc.get();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  // sync clocks
  setInternalClock();
  setRTC();

  // schedule alarm for interupt wakeup
  scheduleAlarm();

  sleep();

}


void loop () {

  Serial.println("loop()");
  
  setDeviceState();

  executeTask();

  sleep();

  delay(1000);
}

/**
 * Task Execution Control Function
 */
void executeTask()
{
  Serial.println("Execute Task..."); 
}

/**
 * Manage Interupt & Device State (Wake/Sleep)
 */
void setDeviceState()
{
  delay(1000);

  if (interuptState == 1) // ISR called
  {
    wakeUp();
    detachInterrupt(digitalPinToInterrupt(rtcInterruptPin));
    interuptState = 0;

    if (alarmInterval >= 1) // Repeat alarm interupt
    {
      scheduleRepeatAlarm(alarmType, alarmInterval);
  
      deviceState ^= 1 << 0; // toggle device state
      if (deviceState == 1) 
      {
        sleep();
      }
    }
  }
}


/**
 * Examples of Alarm scheduling
 */
void scheduleAlarm()
{

  uint8_t i, nextIntervalSec, nextIntervalMin, nextIntervalHour, nextIntervalDay = 0; 
  DateTime dt = rtc.get();

  switch(alarmType)
  {
    case ALM1_EVERY_SECOND :
      setAlarmInterupt(ALM1_EVERY_SECOND , 0, 0, 0, 0);
      break;

    case ALM1_MATCH_SECONDS :
      if (alarmMatchSec >= 1)
      {
        nextIntervalSec = alarmMatchSec;
      } else {
        i = dt.second() + alarmInterval;
        nextIntervalSec = ((i / 60) < 60) ? (i) : 0 + (i - 60) ;
      }
      break;
      
    case ALM1_MATCH_MINUTES :
      if (alarmMatchMin >= 1)
      {
        nextIntervalSec = alarmMatchSec;
        nextIntervalMin = alarmMatchMin;
      } else {
        i = (dt.minute() * 60) + alarmInterval;
        nextIntervalMin = ((i / 60) < 60) ? (i) : 0 + (i - 60) ;
      }

    case ALM1_MATCH_HOURS :
      if (alarmMatchHour >= 1)
      {
        nextIntervalSec = alarmMatchSec;
        nextIntervalMin = alarmMatchMin;
        nextIntervalHour = alarmMatchHour;
      } 
      break;

    case ALM1_MATCH_DAY :
      if (alarmMatchDay >= 1)
      {
        nextIntervalSec = alarmMatchSec;
        nextIntervalMin = alarmMatchMin;
        nextIntervalHour = alarmMatchHour;
        nextIntervalDay = alarmMatchDay;
      } 
      break;

    case ALM1_MATCH_DATE :
      // @todo 
      break;
  }

  setAlarmInterupt(alarmType , nextIntervalSec, nextIntervalMin, nextIntervalHour, nextIntervalDay);
}

/**
 * If alarm_interval (secs|mins) specified, schedule a repeat alarm
 * @param ALARM_TYPES_t ALM1_MATCH_SECONDS | ALM1_MATCH_MINUTES
 * @param int (secs|mins) to next alarm
 */
void scheduleRepeatAlarm(ALARM_TYPES_t alarmType, uint8_t alarmInterval)
{

  if (alarmInterval >= 1)
  {
  
    DateTime dt = rtc.get();
    uint8_t nextIntervalMins, nextIntervalSecs, interval = 0;
    
    switch(alarmType)
    {
      case ALM1_MATCH_SECONDS :
        interval = dt.second() + alarmInterval;
        nextIntervalSecs = (interval < 60) ? (interval) : 0 + (interval - 60) ;
        break;
        
      case ALM1_MATCH_MINUTES :
        interval = (dt.minute() * 60) + alarmInterval;
        nextIntervalMins = ((interval / 60) < 60) ? (interval) : 0 + (interval - 60) ;
        break;
    }

    setAlarmInterupt(alarmType , nextIntervalSecs, nextIntervalMins, 0, 0);
  }

}

/**
 * Set DS3132/3232 Alarm Interupt
 * 
 * @param byte seconds 00-59
 * @param byte minutes 00-59
 * @param byte hours 1-12 + AM/PM | 00-23
 * @param byte daydate 1-7
 */
void setAlarmInterupt(ALARM_TYPES_t alarmType, byte seconds, byte minutes, byte hours, byte daydate)
{
  Serial.println("setAlarmInterupt()");

  char dt[16];
  sprintf(dt, "%02d:%02d:%02d", hours, minutes, seconds);
  Serial.println(dt);

  rtc.squareWave(SQWAVE_NONE);                  // Turns OFF 1Hz Frequency mode on RTC pin SQW
  rtc.alarm( ALARM_1 );
  rtc.alarmInterrupt(ALARM_2, false);           
  rtc.alarmInterrupt(ALARM_1, true);            // Turns on Alarm_1
  rtc.setAlarm(alarmType , seconds, minutes, hours,daydate);

  pinMode( rtcInterruptPin, INPUT_PULLUP );
  attachInterrupt(digitalPinToInterrupt(rtcInterruptPin), (*isrInteruptFunc), FALLING);

}

/**
 * ISR handler for RTC timer
 */
void rtcInterupt()
{
  currentMillis = millis();
  if ((currentMillis - lastInterruptMillis) > interruptWaitPeriod) 
  {
    interuptState = 1;
  }
  lastInterruptMillis = millis();

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
  Serial.println("setInternalClock()");
  DateTime compile_t = DateTime(__DATE__, __TIME__);
  setTime(compile_t.unixtime());
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

  deviceState = 1; // sleeping

  digitalWrite(13, LOW); // turn off internal LED

  cli (); // disable interrupts
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();
  sei (); // enable interrupts
  sleep_cpu();
}

void wakeUp()
{
  sleep_disable();

  digitalWrite(13, HIGH); // turn on internal LED

  Serial.println("wakeUp()");
  delay(1000);
}
