/**
 * Automated Plant Watering
 * 
 * Arduino Pro Mini 3.3v 
 * Capacitive soil moisture sensor v1.2 (pin A0)
 * 5v water pump attached via a relay (pin d7 wired normally closed)
 * OLED SSD1306 124*64 display
 * 4 push button control panel (wired pin d2-d6)
 * 5v power (from solar / battery / 12v DC adapter) via L7805 regulator
 * RTC DS1307
 * Lamp via mains 240v AC relay
 * DIY Capacitive Water Level Sensors (reservoir / overflow)
 * 
 * Notes -
 * Timers (24h) define LAMP on/off and watering (pump) times
 * When soil moisture level is found (dry) pump is activated for n secs
 * Delays are implemented for sensor calibration and between pump activations
 * Config params (pump duration/delay etc) can be modified at runtime,
 * EEPROM is used for persistance
 *
 * @todo -
 *  logic to detect faulty sensor values, deactivate pump
 *
 */

#include <Wire.h>
#include "RTClib.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <EEPROM.h>


RTC_DS1307 rtc;
DateTime dt;

unsigned long startTime = 0;
unsigned long calibrationTime = 15000; // time for sensor(s) to calibrate
unsigned long sampleInterval = 10000;  // sample frequency in ms
unsigned long sampleTimer = 0;

// Pins & mapping to devices
const int r1Pin = 10; // pump relay #1
const int r2Pin = 9;  // lamp relay
const int r3Pin = 11; // pump relay #2
const int s1Pin = A0; // soil moisture sensor #1
const int s2Pin = A1; // soil moisture sensor #2
const int s3inPin = A2; // water level sensor #1 (reservoir)
const int s3outPin = 12; // water level sensor #1
const int s4inPin = A3; // water level sensor #2 (overflow)
const int s4outPin = 13; // water level sensor #2

// Soil Moisture Capacitive v1.2 sensor calibration
//int airVal = 1024;
//int waterVal = 570;
int airVal = 680;
int waterVal = 326;


int interval = 3; // 1-5 higher = increase dryness threshold
const int intervals = (airVal - waterVal)/interval;
int soilMoistureValue = 0;
int soilMoistureStatusId;

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

// config opts: pump, sensor, system
#define CFG_PUMP_ONOFF 20
#define CFG_PUMP_DURATION 21
#define CFG_PUMP_DELAY 22
#define CFG_LAMP 23
#define CFG_AIR_VAL 24
#define CFG_WATER_VAL 25
#define CFG_INTERVALS 26
#define CFG_CALIBRATION_TIME 27
#define CFG_RESET 28

// pointers to config opt index
const int cfNumItems = 9;
const int cfStartIdx = 20;

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


const char l20[] PROGMEM = "Pump On/Off";
const char l21[] PROGMEM = "Pump Duration";
const char l22[] PROGMEM = "Pump Delay";
const char l23[] PROGMEM = "Lamp";
const char l24[] PROGMEM = "Air";
const char l25[] PROGMEM = "Water";
const char l26[] PROGMEM = "Interval";
const char l27[] PROGMEM = "Calib Time";
const char l28[] PROGMEM = "Reset";

const char *const label[] PROGMEM = {l0, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15, ln, ln, ln, ln, l20, l21, l22, l23, l24, l25, l26, l27, l28};

// pump #1
bool pumpActive = 0;
unsigned long pumpDuration = 30000; // pump active duration
unsigned long pumpDelay = 300000; // min time in ms between pump activations
unsigned long pumpLastActivation = 0; // ts of most recent activation
unsigned long pumpActivations = 0; // count of activations

// lamp #1
bool lampActive = 0;
bool lampManualActive = 0; // manual toggle (prevents auto-deactivation)
unsigned long lampLastActivation = 0;
unsigned long lampActivations = 0;
int nextEvent;

// Timer 32 bit bitmask defines hours (from 24h clock) on / off
// 0b 00000000 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

// Lamp On - 06:00 - 20:00
const long lampTimer = 0b00000000000111111111111111000000;

// Watering time(s) - Morning (05:00 - 08:00) Evening (19:00 - 21:00)
const long waterTimer =0b00000000001110000000000111100000;


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


volatile bool irqStatus = 0;
volatile int activeButton = 0;

// buttons correspond to digital pin set bit in PORTD D3-D6
// PIND 0 (01), 1 (02), 2 (04), 3 b1 D3 (0x08), 4 b2 D4 (0x10), 5 b3 D5 (0x20), 6 b4 D6 (0x40)

#define BUTTON_01 0x08
#define BUTTON_02 0x10
#define BUTTON_03 0x20
#define BUTTON_04 0x40

unsigned long buttonDebounce = 75; // msecs
unsigned long buttonLastActive = 0;

bool cfActive = 0;
bool cfModified = 0;
unsigned long cfLastActive = 0;
unsigned long cfActiveDelay = 120000;
int cfSelectedIdx = cfStartIdx;
bool resetStatus = 0;
#define sec 1000
#define tenSec 10000

// PROGMEM utils
char buffer[30];

// read str from progmem
void getText(const char *const  arr[], int i)
{
    strcpy_P(buffer, (char *)pgm_read_word(&(arr[i])));
}

// Timer bitmask - check if bit at pos n is set in 32bit long l
bool checkBitSet(int n, long *l)
{
  bool b = (*l >> (long) n) & 1U;
  return b;
}


// EEPROM status 0: empty (no values), 1: default (initial) values, 2: updated (non default) values
#define EEPROM_EMPTY 0
#define EEPROM_DEFAULT 1
#define EEPROM_MODIFIED 2

void EEPROMInit()
{
  int e = EEPROM.read(0);

  if (e == EEPROM_EMPTY) // persist initial config
  {
    EEPROMWrite(EEPROM_DEFAULT);
  } else if (e == EEPROM_MODIFIED) {
    EEPROMRead();
  }
}

void EEPROMRead()
{
  pumpDuration = EEPROM.read(1);
  pumpDelay = EEPROM.read(2);
  airVal = EEPROM.read(3);
  waterVal = EEPROM.read(4);
  interval = EEPROM.read(5);
  calibrationTime = EEPROM.read(6);
}

void EEPROMWrite(int e)
{ 
  EEPROM.write(0, e);
  EEPROM.write(1, pumpDuration);
  EEPROM.write(2, pumpDelay);
  EEPROM.write(3, airVal);
  EEPROM.write(4, waterVal);
  EEPROM.write(5, interval);
  EEPROM.write(6, calibrationTime);
}


// push button IRQ
void pin2IRQ()
{
  int x;
  x = PIND; // digital pins 0-7

  if ((x & BUTTON_01) == 0) {
    activeButton = BUTTON_01;
  }

  if ((x & BUTTON_02) == 0) {
    activeButton = BUTTON_02;
  }

  if ((x & BUTTON_03) == 0) {
    activeButton = BUTTON_03;
  }

  if ((x & BUTTON_04) == 0) {
    activeButton = BUTTON_04;
  }

  if (activeButton > 0)
  {
    irqStatus = 1;
  }
}

// button event handler
void handleButtonEvent()
{
  if (millis() > buttonLastActive + buttonDebounce)
  {

    if (!displayActive)
    {
      displayOn(); // pressing any button activates display
      return;
    }

    switch (activeButton)
    {
      case BUTTON_01: // edit config
        editConfig();
        break; 
      case BUTTON_02: // exit config / pump ON - OFF
        exitCF();
        break;
      case BUTTON_03: // increase param / display on
        if (cfActive)
        {
          incConfigOpt();
        }
        break;
      case BUTTON_04: // decrease param
        if (cfActive)
        {
          decConfigOpt();
        } else {
          displayOff();
        }
        break;
      default:
        break;
    }
  }

  activeButton = 0;
  buttonLastActive = millis();
  irqStatus = 0;
}

void incConfigOpt()
{
  if (cfActive == 1)
  {
    switch (cfSelectedIdx)
    {
      case CFG_LAMP:
        toggleLamp();
        break;
      case CFG_PUMP_ONOFF:
        togglePump();
        break;
      case CFG_PUMP_DURATION:
        pumpDuration = pumpDuration + tenSec;
        break;
      case CFG_PUMP_DELAY:
        pumpDelay = pumpDelay + tenSec;
        break;
      case CFG_AIR_VAL:
        airVal++;
        break;
      case CFG_WATER_VAL:
        waterVal++;
        break;
      case CFG_INTERVALS:
        if (interval <= 5)
        {
          interval++;
        }
        break;
      case CFG_CALIBRATION_TIME:
        calibrationTime = calibrationTime + sec;
        break;
      case CFG_RESET:
        resetStatus = (resetStatus == 0) ? 1 : 0;
        break;
    }    
  }
  cfLastActive = millis();
  cfModified = 1;

  display(DISPLAY_CONFIG);

}

void decConfigOpt()
{
  if (cfActive == 1)
  {
    switch (cfSelectedIdx)
    {
      case CFG_LAMP:
        toggleLamp();
        break;
      case CFG_PUMP_ONOFF:
        togglePump();
        break;
      case CFG_PUMP_DURATION:
        if (pumpDuration > tenSec)
        {
          pumpDuration = pumpDuration - tenSec;
        } else {
          pumpDuration = 0;
        }
        break;
      case CFG_PUMP_DELAY:
        if (pumpDelay > tenSec)
        {
          pumpDelay = pumpDelay - tenSec;
        } else {
          pumpDelay = 0;
        }
        break;
      case CFG_AIR_VAL:
        if (airVal > 0)
        {
          airVal--;
        }
        break;
      case CFG_WATER_VAL:
        if (waterVal > 0)
        {
          waterVal--;
        }
        break;
      case CFG_INTERVALS:
        if (interval > 0)
        {
          interval--;
        }
        break;
      case CFG_CALIBRATION_TIME:
        if (calibrationTime > sec)
        {
          calibrationTime = calibrationTime - sec;
        } else {
          calibrationTime = 0;
        }
        break;
      case CFG_RESET:
        resetStatus = (resetStatus == 0) ? 1 : 0;
        break;
    }    
  }
  cfModified = 1;
  cfLastActive = millis();
  display(DISPLAY_CONFIG);
}

void editConfig()
{
  if (cfActive == 0)
  {
    cfActive = 1;
  } else {
    if (cfSelectedIdx < (cfStartIdx + cfNumItems -1))
    {
      cfSelectedIdx++;
    } else {
      cfSelectedIdx = cfStartIdx;
    }
  }
  cfLastActive = millis();
  display(DISPLAY_CONFIG);
}

void(* resetFunc) (void) = 0;

void exitCF()
{
  if (cfModified == 1)
  {
    EEPROMWrite(EEPROM_MODIFIED);
  }
  if (resetStatus == 1)
  {
    EEPROMWrite(EEPROM_EMPTY);
    resetStatus = 0;
    resetFunc();
  }
  cfActive = 0;
  cfSelectedIdx = cfStartIdx;
  cfLastActive = 0;
  cfModified = 0;

  display(DISPLAY_STATUS);
}

void displayOn()
{
  oled.ssd1306WriteCmd(SSD1306_DISPLAYON);
  displayActive = 1;
  display(DISPLAY_STATUS);
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

// manual pump on/off toggle
void togglePump()
{
  if (pumpActive == 0)
  {
    pumpActive = 1;
    pumpOn();
  } else {
    pumpActive = 0;
    pumpOff();
  }
}

void pumpOn()
{
    pumpActive = 1;
    digitalWrite(r1Pin, LOW);
}

void pumpOff()
{
    pumpActive = 0;
    digitalWrite(r1Pin, HIGH);
}

// scheduled pump activation for duration pumpDuration
void activatePump()
{

  if (isCalibrating())
  {
    return;
  }

  // check we are within a defined watering time
  bool isScheduled = checkBitSet(dt.hour(), &waterTimer);

  if (!isScheduled)
  {
    pumpOff();
    return;
  }

  if(!pumpActive && (pumpLastActivation == 0 || (millis() > pumpLastActivation + pumpDelay)))
  {
    pumpOn();
    pumpActivations++;
    pumpLastActivation = millis();
  }

}

void deactivatePump()
{
  if (pumpActive && (millis() > pumpLastActivation + pumpDuration))
  {
    pumpOff();
  }
}

// manual lamp on/off toggle
void toggleLamp()
{
  if (lampActive == 0)
  {
    lampActive = 1;
    lampManualActive = 1;
    lampOn();
  } else {
    lampActive = 0;
    lampManualActive = 0;
    lampOff();
  }
}

void lampOn()
{
    lampActive = 1;
    digitalWrite(r2Pin, HIGH);
}

void lampOff()
{
    lampActive = 0;
    digitalWrite(r2Pin, LOW);
}

// scheduled lamp activation / de-activation
void activateLamp()
{
  // check lamp timer for scheduled activation time frame
  dt = rtc.now();
  bool isScheduled = checkBitSet(dt.hour(), &lampTimer);

  if ((isScheduled) && (!lampActive))
  {
    lampOn();
    lampActivations++;
    lampLastActivation = millis();
  } else if (!isScheduled && lampActive && !lampManualActive) {
      lampOff();
  }
}

// return hour of next timer on/off event
int getNextEvent(long *l)
{
  dt = rtc.now();
  bool c = checkBitSet(dt.hour(), *l);

  for(int i=dt.hour(); i<24; i++)
  {
    bool n = checkBitSet(i, &lampTimer);
    if (n != c)
    {
      return i;
    }
  }
  for(int i=0; i<dt.hour(); i++)
  {
    bool n = checkBitSet(i, &lampTimer);
    if (n != c)
    {
      return i;
    }
  }
}

// check water level (reservoir / overflow) sensors
void readWaterLevel()
{
  digitalWrite(s3outPin, HIGH);
  delay(200);
  int v = readSensorAvg(s3inPin, 3, sec);
  digitalWrite(s3outPin, LOW);

  // @todo - define thresholds, actions eg system shutdown if out of water or overflow
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

void readSoilMoisture()
{

  if ((isCalibrating() || pumpActive || cfActive))
  {
    return;
  }

  // take 3 samples with a 200 ms delay
  soilMoistureValue = readSensorAvg(s1Pin, 3, 200);

  if(soilMoistureValue > waterVal && (soilMoistureValue < (waterVal + intervals))) // V Wet
  {
    soilMoistureStatusId = 0;
  }
  else if(soilMoistureValue > (waterVal + intervals) && (soilMoistureValue < (airVal - intervals))) // Wet
  {
    soilMoistureStatusId = 1;
  }
  else if(soilMoistureValue < airVal && (soilMoistureValue > (airVal - intervals))) // Dry
  {
    soilMoistureStatusId = 2;
    activatePump();
  }

  Serial.println(soilMoistureValue);
  Serial.println(airVal);
  Serial.println(waterVal);
  Serial.println(intervals);

  Serial.println(soilMoistureStatusId);
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
    oled.print(F(" "));
    oled.print(soilMoistureValue);
    oled.print(F(" "));
    getText(label, soilMoistureStatusId);
    oled.println(buffer);

    getText(label, CFG_LAMP);
    oled.print(buffer);
    oled.print(F(" "));
    if (lampActive == 1)
    {
      getText(label, LABEL_ON);
      oled.print(buffer);
    } else {
      getText(label, LABEL_OFF);
      oled.print(buffer);
    }
    oled.print(F(" "));
    oled.print(lampActivations);
    oled.print(F(" "));

    nextEvent = getNextEvent(&lampTimer);
    char h[2];
    sprintf(h,"%02d",nextEvent);
    oled.print(h);
    oled.println(":00");

    getText(label, LABEL_PUMP);
    oled.print(buffer);
    oled.print(F(" "));
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

    oled.println("");

    char dtm[32];
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
    oled.println(dtm);

  } else if (opt == DISPLAY_CONFIG) {

    switch (cfSelectedIdx)
    {

      case CFG_LAMP:
        getText(label, CFG_LAMP);
        oled.println(buffer);
        oled.set2X();
        if (lampActive == 1)
        {
          getText(label, LABEL_ON);
          oled.println(buffer);
        } else {
          getText(label, LABEL_OFF);
          oled.println(buffer);
        }
        break;
      case CFG_PUMP_ONOFF:
        getText(label, CFG_PUMP_ONOFF);
        oled.println(buffer);
        oled.set2X();
        if (pumpActive == 1)
        {
          getText(label, LABEL_ON);
          oled.println(buffer);
        } else {
          getText(label, LABEL_OFF);
          oled.println(buffer);          
        }
        break;
      case CFG_PUMP_DURATION:
        getText(label, CFG_PUMP_DURATION);
        oled.println(buffer);
        oled.set2X();
        oled.print(pumpDuration / sec);
        break;

      case CFG_PUMP_DELAY:
        getText(label, CFG_PUMP_DELAY);
        oled.println(buffer);
        oled.set2X();
        oled.print(pumpDelay / sec);
        break;

      case CFG_AIR_VAL:
        getText(label, CFG_AIR_VAL);
        oled.println(buffer);
        oled.set2X();
        oled.print(airVal);
        break;

      case CFG_WATER_VAL:
        getText(label, CFG_WATER_VAL);
        oled.println(buffer);
        oled.set2X();
        oled.print(waterVal);
        break;

      case CFG_INTERVALS:
        getText(label, CFG_INTERVALS);
        oled.println(buffer);
        oled.set2X();
        oled.print(interval);
        break;

      case CFG_CALIBRATION_TIME:
        getText(label, CFG_CALIBRATION_TIME);
        oled.println(buffer);
        oled.set2X();
        oled.print(calibrationTime / sec);
        break;

      case CFG_RESET:
        getText(label, CFG_RESET);
        oled.println(buffer);
        oled.set2X();
        if (resetStatus == 1)
        {
          getText(label, LABEL_YES);
          oled.println(buffer);
        } else {
          getText(label, LABEL_NO);
          oled.println(buffer);          
        }
        break;
    }

    oled.set1X();

  } else if (opt == DISPLAY_CALIB) {

    getText(label, LABEL_STARTMSG);
    oled.println(buffer);

    getText(label, LABEL_SOIL_CALIBRATING);
    oled.println(buffer);

    oled.println("");

    getText(label, CFG_AIR_VAL);
    oled.print(buffer);
    getText(label, CFG_WATER_VAL);
    oled.print(buffer);
    oled.print(F(" "));
    oled.print(airVal);
    oled.print(F(" "));
    oled.print(F(" "));
    oled.print(waterVal);
    oled.print(F(" "));
    oled.println(interval);

    getText(label, CFG_LAMP);
    oled.print(buffer);
    oled.print(F(" "));
    bool isScheduled = checkBitSet(dt.hour(), &lampTimer);

    if (isScheduled == 1)
    {
      getText(label, LABEL_ON);
      oled.print(buffer);
    } else {
      getText(label, LABEL_OFF);
      oled.print(buffer);
    }
    oled.print(F(" "));
    nextEvent = getNextEvent(&lampTimer);
    char h[2];
    sprintf(h,"%02d",nextEvent);
    oled.print(h);
    oled.println(":00");

    getText(label, LABEL_WATERING);
    oled.print(buffer);
    oled.print(F(" "));
    isScheduled = checkBitSet(dt.hour(), &waterTimer);
    if (isScheduled == 1)
    {
      getText(label, LABEL_ON);
      oled.print(buffer);
    } else {
      getText(label, LABEL_OFF);
      oled.print(buffer);
    }
    oled.print(F(" "));
    oled.print(pumpDuration / sec);
    oled.print(F(" "));
    oled.print(pumpDelay / sec);
    oled.print(F(" "));
    nextEvent = getNextEvent(&waterTimer);
    sprintf(h,"%02d",nextEvent);
    oled.print(h);
    oled.println(":00");


  }

  displayLastActivation = millis();
}

void setup() {
  Serial.begin(115200);

  startTime = millis();

  getText(label, LABEL_STARTMSG);
  Serial.print(buffer);

  char dtm[32];
  sprintf(dtm, "  %02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
  Serial.println(dtm);

  Serial.println(airVal);
  Serial.println(waterVal);
  Serial.println(intervals);

  Wire.begin();
  Wire.setClock(400000L);

  rtc.begin();
  DateTime rtc_t = rtc.now();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  // sync RTC w/ arduino time @ compile time
  if (rtc_t.unixtime() < arduino_t.unixtime())
  {
    rtc.adjust(arduino_t.unixtime());
  }

  // Sensor / Relay (pump/lamp) Pins
  pinMode(s1Pin, INPUT);
  pinMode(r1Pin, OUTPUT);
  digitalWrite(r1Pin, HIGH); // switch pump off
  pinMode(r2Pin, OUTPUT);
  pinMode(s3inPin, INPUT);
  pinMode(s3outPin, OUTPUT);
  pinMode(s4inPin, INPUT);
  pinMode(s4outPin, OUTPUT);


  // OLED LCD Display

  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  // Buttons & IRQ
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);

  attachInterrupt(0, pin2IRQ, FALLING);

  // read config values
  EEPROMInit();

  sampleTimer = millis();
}

void loop() {

  // check lamp timer and (de)activate
  activateLamp();

  if (!isCalibrating() && millis() >= sampleTimer + sampleInterval)
  {
  
    // read soil moisture sensor #1
    readSoilMoisture();
  
    // check pump status and (de)activate
    deactivatePump();

    display(DISPLAY_STATUS);

    sampleTimer = millis();
  }

  if (isCalibrating())
  {
    display(DISPLAY_CALIB);
    delay(10000);
  }

  if (irqStatus == 1)
  {
    handleButtonEvent();
  }

  if (cfActive && (millis() > (cfLastActive + cfActiveDelay)))
  {
    exitCF();
  }

  if(millis() > (displayLastActivation + displayTimeout))
  {
    displayOff();
  }

  delay(200);
}
