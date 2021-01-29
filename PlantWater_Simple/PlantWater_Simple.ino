/**
 * Automated Inddor Garden System
 * 
 *  -- Schedule LED grow LAMP for specific hours of day
 *  -- automated watering cycle triggered by reading soil moisture sensor
 * 
 * Arduino Pro Mini 3.3v 
 * 2x Capacitive soil moisture sensor v1.2
 * 2x 5v water pump attached via a relay
 * OLED SSD1306 124*64 display
 * RTC DS1307
 * Lamp via mains 240v AC relay
 * 
 * Simple version with static (compile time) configuration and OLED display
 *
 */

#include <Wire.h>
#include "RTClib.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

#include "Pump.h"
#include "Relay.h"


RTC_DS1307 rtc;
DateTime dt;

unsigned long startTime = 0;
unsigned long calibrationTime = 15000; // time for sensor(s) to calibrate
unsigned long sampleInterval = 10000;  // sample frequency in ms
unsigned long sampleTimer = 0;

// Pins & mapping to devices
const int r1Pin = 10; // pump relay #1
const int r2Pin = 11; // pump relay #2
const int r3Pin = 12; // lamp relay
const int s1Pin = A1; // soil moisture sensor #1
const int s2Pin = A2; // soil moisture sensor #2


/*
 * Define devices and associated timings
 * 
 */

// master on/off switches
bool pumpEnabled = true;
bool lampEnabled = true;

long pumpDuration = 50000;
long pumpDelay = 5000000;

Timer pump1Timer;
// Timer 32 bit bitmask defines hours (from 24h hours) device is on | off
// 0b 00000000 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
long pump1TimerBitmask = 0b00000000000000000000001111100000;
Pump pump1(r1Pin, pumpDuration, pumpDelay); // (<pin-id> <duration> <delay>)

Timer pump2Timer;
long pump2TimerBitmask = 0b00000000000000000000001111100000;
Pump pump2(r2Pin, pumpDuration, pumpDelay);

Timer lamp1Timer;
long lamp1TimerBitmask =0b00000000001111111111111111100000;
Relay lamp1(r3Pin);


// Soil Moisture Capacitive v1.2 sensor calibration
int airVal = 680;
int waterVal = 326;

int interval = 3; // 1-5 higher increase dryness threshold
const int intervals = (airVal - waterVal)/interval;
int soilMoistureVal[2];
int soilMoistureStatusId[2];

// text label ids
#define LABEL_V_WET 0
#define LABEL_WET 1
#define LABEL_DRY 2
#define LABEL_V_DRY 3
#define LABEL_STARTMSG 4
#define LABEL_SOIL_MOISTURE 5
#define LABEL_SOIL_CALIBRATING 6
#define LABEL_PUMP 7
#define LABEL_LAST_ACTIVE 8
#define LABEL_DUR_DELAY 9
#define LABEL_YES 10
#define LABEL_NO 11
#define LABEL_ON 12
#define LABEL_OFF 13
#define LABEL_NEXT 14
#define LABEL_WATERING 15
#define LABEL_PUMPONOFF 16
#define LABEL_PUMPDUR 17
#define LABEL_PUMPDEL 18
#define LABEL_LAMP 19
#define LABEL_AIR 20
#define LABEL_WATER 21
#define LABEL_INTERVAL 22
#define LABEL_CALIB 23 


// string labels (lang = EN GB)
const char ln[] PROGMEM = "";
const char l0[] PROGMEM = "V Wet";
const char l1[] PROGMEM = "Wet";
const char l2[] PROGMEM = "Dry";
const char l3[] PROGMEM = "V Dry";
const char l4[] PROGMEM = "Plant Watering";
const char l5[] PROGMEM = "Soil";
const char l6[] PROGMEM = "Calibrating";
const char l7[] PROGMEM = "Pump";
const char l8[] PROGMEM = "Last Water";
const char l9[] PROGMEM = "Dur/Delay";
const char l10[] PROGMEM = "Yes";
const char l11[] PROGMEM = "No";
const char l12[] PROGMEM = "On";
const char l13[] PROGMEM = "Off";
const char l14[] PROGMEM = "Next";
const char l15[] PROGMEM = "Water";
const char l16[] PROGMEM = "Pump On/Off";
const char l17[] PROGMEM = "Pump Duration";
const char l18[] PROGMEM = "Pump Delay";
const char l19[] PROGMEM = "Lamp";
const char l20[] PROGMEM = "Air";
const char l21[] PROGMEM = "Water";
const char l22[] PROGMEM = "Interval";
const char l23[] PROGMEM = "Calib Time";
const char l24[] PROGMEM = "Reset";

const char *const label[] PROGMEM = {l0, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15, l16, l17, l18, l19, l20, l21, l22, l23, l24};


// SSD1306 Ascii 

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

SSD1306AsciiWire oled;

bool displayActive = 1; // OLED LCD display status
unsigned long displayTimeout = 300000; // delay to turn display off
unsigned long displayLastActivation = 0; // ts of last display activation

#define DISPLAY_STATUS 0x01
#define DISPLAY_CONFIG 0x10
#define DISPLAY_CALIB  0x20

#define sec 1000
#define tenSec 10000


// PROGMEM utils
char buffer[30];

// read str from progmem
void getText(const char *const  arr[], int i)
{
    strcpy_P(buffer, (char *)pgm_read_word(&(arr[i])));
}


void displayOn()
{
  oled.ssd1306WriteCmd(SSD1306_DISPLAYON);
  displayActive = 1;
}

void displayOff()
{
  oled.ssd1306WriteCmd(SSD1306_DISPLAYOFF);
  displayActive = 0;
}

bool isCalibrating()
{
  return (millis() > startTime + calibrationTime) ? false : true;
}


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

  if(*v > waterVal && (*v < (waterVal + intervals))) // V Wet
  {
    *id = 0;
  }
  else if(*v > (waterVal + intervals) && (*v < (airVal - intervals))) // Wet
  {
    *id = 1;
  }
  else if(*v < airVal && (*v > (airVal - intervals))) // Dry
  {
    *id = 2;
  }
}

void display(int opt)
{

  if (displayActive == 0) return;

  oled.setFont(Adafruit5x7);
  oled.clear();

  if (opt == DISPLAY_STATUS)
  {

    getText(label, LABEL_SOIL_MOISTURE);
    oled.print(buffer);
    oled.print(F("#1 "));
    oled.print(soilMoistureVal[0]);
    oled.print(F(" "));
    getText(label, soilMoistureStatusId[0]);
    oled.println(buffer);

    getText(label, LABEL_SOIL_MOISTURE);
    oled.print(buffer);
    oled.print(F("#2 "));
    oled.print(soilMoistureVal[1]);
    oled.print(F(" "));
    getText(label, soilMoistureStatusId[1]);
    oled.println(buffer);

    getText(label, LABEL_LAMP);
    oled.print(buffer);
    oled.print(F(" "));
    if (lamp1.isActive())
    {
      getText(label, LABEL_ON);
      oled.print(buffer);
    } else {
      getText(label, LABEL_OFF);
      oled.print(buffer);
    }
    oled.print(F(" "));
    oled.print(lamp1.getActivations());
    oled.print(F(" "));

    int nextEvent = lamp1.timer.getNextEvent(dt.hour());
    char h[2];
    sprintf(h,"%02d",nextEvent);
    oled.print(h);
    oled.println(":00");

    getText(label, LABEL_PUMP);
    oled.print(buffer);
    oled.print(F(" "));
    /*
    if (pumpActive == 1)
    {
      getText(label, LABEL_ON);
      oled.print(buffer);
    } else {
      getText(label, LABEL_OFF);
      oled.print(buffer);
    }
    oled.print(F(" "));
    oled.println(pumpActivations);
    getText(label, LABEL_LAST_ACTIVE);
    oled.print(buffer);
    oled.print(F(" "));
    if (pumpLastActivation > 0)
    {
      oled.println((millis() - pumpLastActivation) / sec);
    } else {
      oled.println(0);
    }
    */

    oled.println("");

    char dtm[32];
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
    oled.println(dtm);

  } else if (opt == DISPLAY_CALIB) {

    getText(label, LABEL_STARTMSG);
    oled.println(buffer);

    getText(label, LABEL_SOIL_CALIBRATING);
    oled.println(buffer);

    oled.println("");

  }

  displayLastActivation = millis();
}

void setup() {
  Serial.begin(115200);

  startTime = millis();

  getText(label, LABEL_STARTMSG);
  Serial.print(buffer);

  Wire.begin();
  Wire.setClock(400000L);

  rtc.begin();
  dt = rtc.now();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  // sync RTC w/ arduino time @ compile time
  if (dt.unixtime() < arduino_t.unixtime())
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


  pump1Timer.init(TIMER_HOUR_OF_DAY, &pump1TimerBitmask);
  pump1.initTimer(pump1Timer);
  
  pump2Timer.init(TIMER_HOUR_OF_DAY, &pump2TimerBitmask);
  pump2.initTimer(pump2Timer);
  
  lamp1Timer.init(TIMER_HOUR_OF_DAY, &lamp1TimerBitmask);
  lamp1.initTimer(lamp1Timer);


  // OLED LCD Display
  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  sampleTimer = millis();
}

void loop() {

  if (!isCalibrating() && millis() >= sampleTimer + sampleInterval)
  {

    // check lamp status for current hour and (de)activate
    dt = rtc.now();

    if (lampEnabled)
    {
      if (!lamp1.isActive() && lamp1.timer.isScheduled(dt.hour(), NULL)) {
        lamp1.on();
      } else if (lamp1.isActive() && !lamp1.timer.isScheduled(dt.hour(), NULL)) {
        lamp1.off();
      }
    }
  
    // read soil moisture - soilMoistureStatusId: { V_WET 0, WET 1, DRY 2, V_DRY 3 }

    // sensor #1
    readSoilMoisture(s1Pin, &soilMoistureVal[0], &soilMoistureStatusId[0]);
    if (soilMoistureStatusId[0] >= 2)
    {
      pump1.activate(dt.hour(), NULL);
    }

    // sensor #2
    readSoilMoisture(s2Pin, &soilMoistureVal[1], &soilMoistureStatusId[1]);
    if (soilMoistureStatusId[0] >= 2)
    {
      pump2.activate(dt.hour(), NULL);
    }

    // check for pump deactivation
    pump1.deactivate(dt.hour(), NULL);
    pump2.deactivate(dt.hour(), NULL);

    display(DISPLAY_STATUS);

    sampleTimer = millis();
  }

  if (isCalibrating())
  {
    display(DISPLAY_CALIB);
    delay(10000);
  }


  delay(200);
}
