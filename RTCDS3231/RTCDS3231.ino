/** 
 * Arduino RTC DS3231/3232
 * 
 * DataSheet: https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
 * 
 * @requires DateTime.h from: https://github.com/steveio/RTCLib
 * 
 * @todo -
 * use mktime() in time.h to parse compile time to unixtimestamp
 * Support all alarm types - ALM2, ALM3, ALM4, with/out interupt
 * 
 */

#include <Wire.h>
#include <DS3232RTC.h>  
#include <Time.h>
#include <DateTime.h>


// DS3232 lib should be compatible with DS3231
DS3232RTC rtc;


#define RTC_SDA_PIN 18                         
#define RTC_SCL_PIN 19  

int rtcInterruptPin = 2;
volatile int8_t int_flag = 0;

int alarm_interval = 0; // Interval in mins for repeating alarm

void setup () {

  Serial.begin(115200);
  Wire.begin();

  Serial.println("RTCDS3231 Begin!");

  pinMode(RTC_SDA_PIN, INPUT);
  pinMode(RTC_SCL_PIN, INPUT);

  rtc.begin();

  //time_t rtc_t = rtc.get();
  DateTime rtc_t = rtc.get();

  Serial.println(__DATE__);
  Serial.println(__TIME__);

  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  Serial.println("RTC:");
  Serial.println(rtc_t.unixtime());
  Serial.println("DateTimeLib:");
  Serial.println(arduino_t.unixtime());

  /**
  RTCDS3231 Begin!
  Feb 25 2020
  21:54:49
  RTC:
  1595880329
  DateTimeLib:
  1582667689
  */

  getLastClockSync();

  setInternalClock();
  setRTC(); // sync RTC clock w/ system time

  scheduleAlarm();
}


void loop () {

  DateTime now = rtc.get();

  if (int_flag)
  {
    Serial.println("RTC Interupt");
    Serial.println(rtc.get());
    int_flag = 0;
    scheduleRepeatAlarm();
  }
  
  char dt[16], tm[16], dtm[32];

  sprintf(dt, "%02d/%02d/%02d", now.day(),now.month(),now.year());
  sprintf(tm, "%02d:%02d:%02d", now.hour(),now.minute(),now.second());
  sprintf(dtm, "%02d/%02d/%02d %02d:%02d:%02d" , now.day(),now.month(),now.year(),now.hour(),now.minute(),now.second());

  // unix timestamp, useful for logging and date/time calculation
  uint32_t ts =now.unixtime();

  Serial.println(tm);
  delay(1000);
}


/**
 * Examples of Alarm scheduling
 */
void scheduleAlarm()
{
  
  /*
   * Alarm Schedule Constants:
  
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

  // every second
  //setAlarmInterupt(ALM1_EVERY_SECOND , 0, 0, 0, 0);

  // seconds match n
  //setAlarmInterupt(ALM1_MATCH_SECONDS , 30, 0, 0, 0);

  // every <interval> minutes
  alarm_interval = 2; // every mins
  setAlarmInterupt(ALM1_MATCH_MINUTES , 0, 30, 0, 0);

  // every hour @ n minutes past
  //setAlarmInterupt(ALM1_MATCH_MINUTES , 0, 59, 0, 0);

  // Day of week 1-7 is (1 = Monday?)

}

/**
 * If alarm_interval (mins) is specified, schedule a repeat alarm
 */
void scheduleRepeatAlarm()
{
    delay(1000); // required to get accurate minute() rollover value!?

    if (alarm_interval > 0)
    {
      int nextInterval = ((minute() + alarm_interval) < 60) ? (minute() + alarm_interval) : 0;
      Serial.println("Repeat Alarm:");
      Serial.println(nextInterval);
      setAlarmInterupt(ALM1_MATCH_MINUTES , 0, nextInterval, 0, 0);
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
  attachInterrupt(digitalPinToInterrupt(rtcInterruptPin), rtcInterupt, FALLING);

}

/**
 * ISR handler for RTC
 */
void rtcInterupt()
{
  int_flag = 1;
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


/* @deprecated - does not provide second resolution */
time_t cvt_date(char const *date) { 
    char s_month[5];
    int month, day, year;
    tmElements_t tmel;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf(date, "%s %d %d", s_month, &day, &year);

    month = (strstr(month_names, s_month)-month_names)/3+1;

    tmel.Hour = tmel.Minute = tmel.Second = 0; // This was working perfectly until 3am then broke until I added this.
    tmel.Month = month;
    tmel.Day = day;
 // year can be given as full four digit year or two digts (2010 or 10 for 2010);
 //it is converted to years since 1970
  if( year > 99)
      tmel.Year = year - 1970;
  else
      tmel.Year = year + 30;

    return makeTime(tmel);
}
