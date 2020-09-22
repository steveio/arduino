/**
 * Automated Plant Watering
 * 
 * Arduino Pro Mini 3.3v 
 * Capacitive soil moisture sensor v1.2 (pin A0)
 * 5v water pump attached via a relay (pin d7 wired normally closed)
 * OLED SSD1306 124*64 display
 * 4 push button control panel (wired pin d2-d6)
 * 5v power (from solar / 12v adapter) via L7805 regulator
 * 
 * When soil moisture level is found high (dry) pump is activated for n secs
 * Delays are implemented for sensor calibration and pump activations
 * Config params (pump duration/delay etc) can be modified at runtime,
 * EEPROM is used for persistance
 *
 * @todo - 
 *  logic to detect faulty sensor values, deactivate pump
 *  out of water / overflow sensor
 *  sleep / wake / low power mode
 *
 */

#include <EEPROM.h>

// sensor calibration
int airVal = 1024;
int waterVal = 570;

const int soilSensorPin = A0;
unsigned long startTime = 0;
unsigned long calibrationTime = 15000; // time for sensor to calibrate (level)
int interval = 2; // 1-5   
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

// config opts: pump, sensor, system
#define CFG_PUMP_ONOFF 14
#define CFG_PUMP_DURATION 15
#define CFG_PUMP_DELAY 16
#define CFG_AIR_VAL 17
#define CFG_WATER_VAL 18
#define CFG_INTERVALS 19
#define CFG_CALIBRATION_TIME 20
#define CFG_RESET 21

// pointers to config opt index
const int cfNumItems = 8;
const int cfStartIdx = 14;

// string labels (lang = EN GB)
const char l0[] PROGMEM = "V Wet";
const char l1[] PROGMEM = "Wet";
const char l2[] PROGMEM = "Dry";
const char l3[] PROGMEM = "V Dry";
const char l4[] PROGMEM = "Plant Watering";
const char l5[] PROGMEM = "Soil Moisture";
const char l6[] PROGMEM = "Calibrating";
const char l7[] PROGMEM = "Pump";
const char l8[] PROGMEM = "Last Active";
const char l9[] PROGMEM = "Dur/Delay";
const char l10[] PROGMEM = "Yes";
const char l11[] PROGMEM = "No";
const char l12[] PROGMEM = "On";
const char l13[] PROGMEM = "Off";
const char l14[] PROGMEM = "Pump On/Off";
const char l15[] PROGMEM = "Pump Duration";
const char l16[] PROGMEM = "Pump Delay";
const char l17[] PROGMEM = "Air Val";
const char l18[] PROGMEM = "Water Val";
const char l19[] PROGMEM = "Wet/Dry Int";
const char l20[] PROGMEM = "Calib Time";
const char l21[] PROGMEM = "Reset";

const char *const label[] PROGMEM = {l0, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15, l16, l17, l18, l19, l20, l21};

const int relayPin = 9;
bool pumpActive = 0;
unsigned long pumpDuration = 30000; // pump active duration
unsigned long pumpDelay = 300000; // min time in ms between pump activations
unsigned long pumpLastActivation = 0; // ts of most recent pump activation
unsigned long pumpActivations = 0;

// SSD1306 Ascii 
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

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

void getText(const char *const  arr[], int i)
{
    strcpy_P(buffer, (char *)pgm_read_word(&(arr[i])));  // Necessary casts and dereferencing, just copy.  
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

    if (displayActive == 0)
    {
      displayOn(); // pressing any button activates display
    }

    switch (activeButton)
    {
      case BUTTON_01: // edit config
        editConfig();
        break; 
      case BUTTON_02: // exit config / pump ON - OFF
        if (cfActive == 1)
        {
          exitCF();
        } else {
          (pumpActive == 0) ? pumpOn() : pumpOff();
        }
        break;
      case BUTTON_03: // increase param / display on
        if (cfActive == 1)
        {
          incConfigOpt();
        }
        break;
      case BUTTON_04: // decrease param
        if (cfActive == 1)
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
      case CFG_PUMP_ONOFF:
        if (pumpActive == 0)
        {
          pumpActive = 1;
          pumpOn();
        } else {
          pumpActive = 0;
          pumpOff();          
        }
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
      case CFG_PUMP_ONOFF:
        if (pumpActive == 0)
        {
          pumpActive = 1;
          pumpOn();
        } else {
          pumpActive = 0;
          pumpOff();          
        }
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
    EEPROMWrite(EEPROM_DEFAULT);
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

void pumpOn()
{
    pumpActive = 1;
    digitalWrite(relayPin, LOW);  
}

void pumpOff()
{
    pumpActive = 0;
    digitalWrite(relayPin, HIGH);
}

// scheduled pump activation for duration pumpDuration
void activatePump()
{
  if(!isCalibrating() && (pumpLastActivation == 0 || millis() > pumpLastActivation + pumpDelay))
  {
    pumpOn();
    display(DISPLAY_STATUS);

    delay(pumpDuration);

    pumpOff();        
    display(DISPLAY_STATUS);

    pumpActivations++;
    pumpLastActivation = millis();
  }
}

void readSoilMoisture()
{
  // take 3 samples with a 1 second delay
  for(int i=0; i<3; i++)
  {
    soilMoistureValue += analogRead(soilSensorPin);
    delay(sec);
  }
  soilMoistureValue = soilMoistureValue / 3;

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

  display(DISPLAY_STATUS);
}

void display(int opt)
{

  if (displayActive == 0) return;

  oled.setFont(Adafruit5x7);
  oled.clear();

  if (opt == DISPLAY_STATUS)
  {
    getText(label, LABEL_SOIL_MOISTURE);
    oled.println(buffer);

    oled.set2X();
    if (isCalibrating())
    {
      getText(label, LABEL_SOIL_CALIBRATING);
      oled.println(buffer);
    } else {
      getText(label, soilMoistureStatusId);
      oled.print(soilMoistureValue);
      oled.print(" ");
      oled.println(buffer);
    }
    oled.set1X();
    oled.println();

    getText(label, LABEL_PUMP);
    oled.print(buffer);
    oled.print(F(" "));
    oled.print(pumpActive);
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

    getText(label, LABEL_DUR_DELAY);
    oled.print(buffer);
    oled.print(F(" "));
    oled.print(pumpDuration / sec);
    oled.print(F(" "));
    oled.println(pumpDelay / sec);

  } else if (opt == DISPLAY_CONFIG) {

    switch (cfSelectedIdx)
    {

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

        /*
        Serial.print(label[CFG_PUMP_DURATION]);
        Serial.print("\t");
        Serial.println(pumpDuration);
        */
        break;

      case CFG_PUMP_DELAY:
        getText(label, CFG_PUMP_DELAY);
        oled.println(buffer);
        oled.set2X();
        oled.print(pumpDelay / sec);

        /*
        Serial.print(label[CFG_PUMP_DELAY]);
        Serial.print("\t");
        Serial.println(pumpDelay);
        */
        break;

      case CFG_AIR_VAL:
        getText(label, CFG_AIR_VAL);
        oled.println(buffer);
        oled.set2X();
        oled.print(airVal);

        /*
        Serial.print(label[CFG_AIR_VAL]);
        Serial.print("\t");
        Serial.println(airVal);
        */
        break;

      case CFG_WATER_VAL:
        getText(label, CFG_WATER_VAL);
        oled.println(buffer);
        oled.set2X();
        oled.print(waterVal);

        /*
        Serial.print(label[CFG_WATER_VAL]);
        Serial.print("\t");
        Serial.println(waterVal);
        */
        break;

      case CFG_INTERVALS:
        getText(label, CFG_INTERVALS);
        oled.println(buffer);
        oled.set2X();
        oled.print(interval);

        /*
        Serial.print(label[CFG_INTERVALS]);
        Serial.print("\t");
        Serial.println(interval);
        */
        break;

      case CFG_CALIBRATION_TIME:
        getText(label, CFG_CALIBRATION_TIME);
        oled.println(buffer);
        oled.set2X();
        oled.print(calibrationTime / sec);

        /*
        Serial.print(label[CFG_CALIBRATION_TIME]);
        Serial.print("\t");
        Serial.println(calibrationTime);
        */
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

  }

  displayLastActivation = millis();
}

void setup() {
  Serial.begin(115200);

  startTime = millis();

  getText(label, LABEL_STARTMSG);
  Serial.println(buffer);

  // Sensor / Pump (relay) Pins
  pinMode(soilSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // switch pump off

  // OLED LCD Display
  Wire.begin();
  Wire.setClock(400000L);

  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  display(DISPLAY_STATUS);

  // Buttons & IRQ
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);

  attachInterrupt(0, pin2IRQ, FALLING);

  // read config values
  EEPROMInit();
}

void loop() {
  if (irqStatus == 1)
  {
    handleButtonEvent();
  }
  if (millis() > (cfLastActive + cfActiveDelay))
  {
    exitCF();
  }

  if (!isCalibrating() && pumpActive != 1 && cfActive != 1)
  {
    readSoilMoisture();
  }

  if(millis() > (displayLastActivation + displayTimeout))
  {
    displayOff();
  }

  delay(200);
}
