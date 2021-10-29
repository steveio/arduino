/** 
 *  Smart Indoor Gardening
 * 
 *  -- Scheduled (relay) activation for LED LAMP(s)
 *  -- Scheduled 5v pump activation for watering
 *  -- Soil moisture / temperature monitoring
 * 
 * Arduino Pro Mini 3.3v 
 * 2x 5v water pump attached via relay
 * RTC DS1307
 * Lamp via mains 240v AC relay
 * 2x Soil Moisture Capacitive v1.2 sensor
 * 
 * Minimal (low memory) standalone mcu optimised
 *
 * @requires Library TimedDevice ( https://github.com/steveio/TimedDevice )
 *
 */

#include <Wire.h>
#include "RTClib.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

// SSD1306 Ascii 

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

SSD1306AsciiWire oled;


#include "RecurringTimer.h"
#include "Pump.h"
#include "Relay.h"


RTC_DS1307 rtc;
DateTime dt;

#define VERSION_ID 1.0

unsigned long startTime = 0;
unsigned long calibrationTime = 1500; // time for sensor(s) to calibrate
unsigned long sampleInterval = 10000;  // sample frequency in ms
unsigned long sampleTimer = 0;


/*
 * Define devices and associated timings
 * 
 */

// Pins & mapping to devices
const int r1Pin = 10; // pump relay #1
const int r2Pin = 11; // pump relay #2
const int r3Pin = 12; // lamp relay
const int s1Pin = A1; // soil moisture sensor #1
const int s2Pin = A2; // soil moisture sensor #2


// master on/off switches
bool pumpEnabled = true;
bool lampEnabled = true;


/*
 * Pump timer schedule uses a combination of:
 *  Hour of day bitmask + duration + delay to define activation window
 *  Recurring timer active 1 day every n days 
 */
long pumpDuration = 180000; // 3 minute activation
long pumpDelay = 3600000; // 1 hour delay between activations

// timer is active 1 day every n days
unsigned long pumpTimerInterval = (SECS_PER_DAY * 1000) * 2;
unsigned long pumpTimerDuration = (SECS_PER_DAY * 1000);

// Timer 32 bit bitmask defines hours (24h) device is on | off
// 0b 00000000 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
long pumpTimerHourBitmask = 0b00000000000000000000001000000000; // 9am 


RecurringTimer pump1Timer;
Pump pump1(r1Pin, pumpDuration, pumpDelay); // (<pin-id> <duration> <delay>)

RecurringTimer pump2Timer;
Pump pump2(r2Pin, pumpDuration, pumpDelay);


/*
 * Lamp timer specifies activation for specific hours recurring every day
 * 
 */

Timer lamp1Timer;
long lamp1TimerBitmask =0b00000000000111111111111111000000; // 6am - 8pm
Relay lamp1(r3Pin);


// Soil Moisture Capacitive v1.2 sensor calibration
int airVal = 680;
int waterVal = 326;
const int intervals = (airVal - waterVal) / 3;

int soilMoistureVal[2];
int soilMoistureStatusId[2];


// take an average from a sensor returning an int value
int readSensorAvg(int pinId, int numSample, int delayTime)
{
  int v = 0;
  for(int i=0; i<numSample; i++)
  {
    v += analogRead(pinId);
    delay(delayTime);
  }

  v = v / numSample;
  return v;
}

/**
 * Read & categorise soil moisture sensor data 
 * @param int pin id
 * @param int soil moisture value (from sensor reading)
 * @param int soil moisture level: { V_WET 0, WET 1, DRY 2, V_DRY 3 }
 */
void readSoilMoisture(int pinId, int * v, int * id)
{

  // take 3 samples with a 200 ms delay
  *v = readSensorAvg(pinId, 3, 200);

  if(*v < airVal && (*v > (airVal - intervals))) // Dry  > 562
  {
    *id = 2;
  }
  else if(*v < airVal && (*v > (airVal - intervals * 2))) // Wet > 444 - 562
  {
    *id = 1;
  }
  else
  {
    *id = 0; // V Wet 326 - 444
  }

}


void displayOLED()
{

    oled.clear();
    oled.set1X();

    char dtm[32];
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
    oled.println(dtm);

    oled.print(F("LA: "));
    if (lamp1.isActive())
    {
      oled.print("1");
    } else {
      oled.print("0");
    }

    oled.println("");

    oled.print(F("LS: "));

    if (lamp1.timer.isScheduled(dt.hour()))
    {
      oled.print("1");
    } else {
      oled.print("0");
    }

    oled.println();

    oled.print(F("LN: "));

    int nextEvent = lamp1.timer.getNextEvent(dt.hour());
    char h[2];
    sprintf(h,"%02d",nextEvent);
    oled.print(h);
    oled.println(":00");
    oled.println("");

}

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

  // Sensor / Relay (pump/lamp) Pins
  pinMode(s1Pin, INPUT);
  pinMode(s2Pin, INPUT);

  pinMode(r1Pin, OUTPUT);
  digitalWrite(r1Pin, HIGH);
  pinMode(r2Pin, OUTPUT);
  digitalWrite(r2Pin, HIGH);

  pinMode(r3Pin, OUTPUT);
  pinMode(r3Pin, HIGH);

  pump1Timer.init(TIMER_MILLIS_RECURRING, &pumpTimerHourBitmask, pumpTimerInterval, pumpTimerDuration, dt.unixtime());
  pump1.initTimer(pump1Timer);
  
  pump2Timer.init(TIMER_MILLIS_RECURRING, &pumpTimerHourBitmask, pumpTimerInterval, pumpTimerDuration, dt.unixtime());
  pump2.initTimer(pump2Timer);
  
  lamp1Timer.init(TIMER_HOUR_OF_DAY, &lamp1TimerBitmask);
  lamp1.initTimer(lamp1Timer);

  // OLED LCD Display
  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  oled.setFont(Adafruit5x7);
  oled.clear();

  char dtm[32];
  sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
  oled.println(dtm);

  sampleTimer = millis();
}

void loop() {


  if (millis() >= sampleTimer + sampleInterval)
  {

    char dtm[32];
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());

    Serial.println(dtm);

    // check lamp status for current hour and (de)activate
    dt = rtc.now();


    displayOLED();

    if (lampEnabled)
    {

      Serial.println("LE");
      Serial.print("LA ");
      Serial.println(lamp1.isActive());
      Serial.print("LS ");
      Serial.println(lamp1.timer.isScheduled(dt.hour(), NULL));

      Serial.print("LN ");

      int nextEvent = lamp1.timer.getNextEvent(dt.hour());
      char h[2];
      sprintf(h,"%02d",nextEvent);
      Serial.print(h);
      Serial.println(":00");


      if (lamp1.timer.isScheduled(dt.hour(), NULL)) {
        Serial.println("L1");
        lamp1.on();
      } else if (!lamp1.timer.isScheduled(dt.hour(), NULL)) {
        Serial.println("L0");
        lamp1.off();
      }
    } else {
      Serial.println("LD");
    }

    // read soil moisture sensors: soilMoistureStatusId: { V_WET 0, WET 1, DRY 2, V_DRY 3 }
    readSoilMoisture(s1Pin, &soilMoistureVal[0], &soilMoistureStatusId[0]);
    readSoilMoisture(s2Pin, &soilMoistureVal[1], &soilMoistureStatusId[1]);


    if (pumpEnabled && pump1.timer.isScheduled(dt.hour(), dt.unixtime()))
    {
      Serial.println("PE");
      pump1.activate(dt.hour());
    } else {
      Serial.println("PD");
    }

    if (pumpEnabled && pump1.timer.isScheduled(dt.hour(), dt.unixtime()))
    {
      pump2.activate(dt.hour());
    }

    // check for pump deactivation
    pump1.deactivate(dt.hour(), dt.dayOfTheWeek());
    pump2.deactivate(dt.hour(), dt.dayOfTheWeek());

    sampleTimer = millis();
  }

}
