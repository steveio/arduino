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
#include <AlarmSchedule.h>
#include <avr/sleep.h>
#include <avr/power.h>

DS3232RTC rtc;

#define RTC_SDA_PIN 18
#define RTC_SCL_PIN 19
#define RTC_INTERRUPT_PIN 2

volatile int8_t interuptState = 0;

AlarmScheduleRTC3232 als;


void setup () {

  Serial.begin(115200);
  Serial.println("RTCDS3231 Begin!");

  Wire.begin();

  pinMode(RTC_SDA_PIN, INPUT);
  pinMode(RTC_SCL_PIN, INPUT);

  pinMode(13,OUTPUT);
  digitalWrite(13, HIGH);

  rtc.begin();

  Serial.print(__DATE__);
  Serial.println(__TIME__);

  DateTime rtc_t = rtc.get();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  // sync clocks
  setInternalClock();
  setRTC();

  pinMode( RTC_INTERRUPT_PIN, INPUT_PULLUP);

  setAlarm();
  sleep();
}

void setAlarm()
{
  Serial.println("setAlarm");
  als.setAlarm( ALM1_MATCH_SECONDS, 30, 0, 0, 0, 0);
}

void loop () {

  DateTime now = rtc.get();
  char t[16];
  sprintf(t, "%02d:%02d:%02d", now.hour(),now.minute(),now.second());
  Serial.println(t);

  if (interuptState == 1) // ISR called
  {
    interuptState = 0;

    executeTask();
    setAlarm();

    sleep();
  }

  delay(900);
}

/**
 * Task Execution Control Function
 */
void executeTask()
{
  Serial.println("Execute Task...");
}


/**
 * ISR handler for RTC timer
 */
void rtcInteruptFunc()
{
  interuptState = 1;
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
}
