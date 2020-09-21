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
 *
 * @todo - 
 *  logic to detect faulty sensor values, deactivate pump
 *  out of water / overflow sensor
 *  sleep / wake / low power mode
 *
 */


// sensor calibration
int airVal = 1024;
int waterVal = 570;

const int soilSensorPin = A0;
unsigned long startTime = 0;
unsigned long calibrationDelay = 15000; // time for sensor to calibrate (level)
int interval = 2; // 1-5   
const int intervals = (airVal - waterVal)/interval;
int soilMoistureValue = 0;

int soilMoistureStatusId;

const char l0[] = "V Wet";
const char l1[] = "Wet";
const char l2[] = "Dry";
const char l3[] = "V Dry";

const char *const label[] = {l0, l1, l2, l3};

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

// config opts: pump, sensor, system
#define CFG_PUMP_DURATION 0x00
#define CFG_PUMP_DELAY 0x01
#define CFG_AIR_VAL 0x02
#define CFG_WATER_VAL 0x03
#define CFG_INTERVALS 0x04
#define CFG_CALIBRATION_TIME 0x05

const char l10[] = "Pump Duration";
const char l11[] = "Pump Delay";
const char l12[] = "Air Val";
const char l13[] = "Water Val";
const char l14[] = "Wet/Dry Int";
const char l15[] = "Calib Time";

const char *const cflabel[] = {l10, l11, l12, l13, l14, l15};

int cfNumItems = 6;
bool cfActive = 0;
unsigned long cfLastActive = 0;
unsigned long cfActiveDelay = 120000;
int cfSelectedItem = 0;
#define sec 1000
#define tenSec 10000

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

    displayOn(); // pressing any button activates display

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

void displayOn()
{
  Serial.println("LCD on");
  oled.ssd1306WriteCmd(SSD1306_DISPLAYON);
  displayActive = 1; 
}

void displayOff()
{
  Serial.println("LCD off");
  oled.ssd1306WriteCmd(SSD1306_DISPLAYOFF);
  displayActive = 0;
}

void incConfigOpt()
{
  if (cfActive == 1)
  {
    switch (cfSelectedItem)
    {
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
        calibrationDelay = calibrationDelay + sec;
        break;
    }    
  }
  display(DISPLAY_CONFIG);
  cfLastActive = millis();
}

void decConfigOpt()
{
  if (cfActive == 1)
  {
    switch (cfSelectedItem)
    {
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
        if (calibrationDelay > sec)
        {
          calibrationDelay = calibrationDelay - sec;
        } else {
          calibrationDelay = 0;
        }
        break;
    }    
  }
  display(DISPLAY_CONFIG);
  cfLastActive = millis();
}

void editConfig()
{
  if (cfActive == 0)
  {
    cfActive = 1;
  } else {
    if (cfSelectedItem < cfNumItems -1)
    {
      cfSelectedItem++;
    } else {
      cfSelectedItem = 0;
    }
  }
  cfLastActive = millis();
  display(DISPLAY_CONFIG);
}

void exitCF()
{
  cfActive = 0;
  cfSelectedItem = 0;
  cfLastActive = 0;
  display(DISPLAY_STATUS);
}

bool isCalibrating()
{
  return (millis() > startTime + calibrationDelay) ? false : true;
}

void pumpOn()
{
    pumpActive = 1;
    digitalWrite(relayPin, LOW);  

    /*
    Serial.print(millis());
    Serial.print("\t");
    Serial.println("Pump On:");
    */
    display(DISPLAY_STATUS);
}

void pumpOff()
{
    pumpActive = 0;
    digitalWrite(relayPin, HIGH);

    /*
    Serial.print(millis());
    Serial.print("\t");
    Serial.println("Pump Off:");
    */
    display(DISPLAY_STATUS);

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
    Serial.print(millis());
    Serial.print("\t");
    Serial.print(soilMoistureValue);
    Serial.print("\t");
    Serial.println(label[soilMoistureStatusId]);
    
    oled.println(F("Soil Moisture: "));
    oled.set2X();
    if (isCalibrating())
    {
      oled.println(F("Calibrating.."));
    } else {
      oled.print(soilMoistureValue);
      oled.print(" ");
      oled.println(label[soilMoistureStatusId]);
    }
    oled.set1X();
    oled.println();
    oled.print(F("Pump:"));
    oled.print(pumpActive);
    oled.print(F(" "));
    oled.println(pumpActivations);
  
    oled.print(F("Pump Last:"));
    if (pumpLastActivation > 0)
    {
      oled.println((millis() - pumpLastActivation) / sec);
    } else {
      oled.println(0);
    }
    oled.print(F("Dur/Delay:"));
    oled.print(pumpDuration / sec);
    oled.print(F(" "));
    oled.println(pumpDelay / sec);

  } else if (opt == DISPLAY_CONFIG) {

    switch (cfSelectedItem)
    {
      case CFG_PUMP_DURATION:
        oled.println(cflabel[CFG_PUMP_DURATION]);
        oled.set2X();
        oled.print(pumpDuration / sec);

        /*
        Serial.print(cflabel[CFG_PUMP_DURATION]);
        Serial.print("\t");
        Serial.println(pumpDuration);        
        */
        break;

      case CFG_PUMP_DELAY:
        oled.println(cflabel[CFG_PUMP_DELAY]);
        oled.set2X();
        oled.print(pumpDelay / sec);

        /*
        Serial.print(cflabel[CFG_PUMP_DELAY]);
        Serial.print("\t");
        Serial.println(pumpDelay);
        */
        break;

      case CFG_AIR_VAL:
        oled.println(cflabel[CFG_AIR_VAL]);
        oled.set2X();
        oled.print(airVal);

        /*
        Serial.print(cflabel[CFG_AIR_VAL]);
        Serial.print("\t");
        Serial.println(airVal);
        */
        break;

      case CFG_WATER_VAL:
        oled.println(cflabel[CFG_WATER_VAL]);
        oled.set2X();
        oled.print(waterVal);

        /*
        Serial.print(cflabel[CFG_WATER_VAL]);
        Serial.print("\t");
        Serial.println(waterVal);
        */
        break;

      case CFG_INTERVALS:
        oled.println(cflabel[CFG_INTERVALS]);
        oled.set2X();
        oled.print(interval);

        /*
        Serial.print(cflabel[CFG_INTERVALS]);
        Serial.print("\t");
        Serial.println(interval);
        */
        break;

      case CFG_CALIBRATION_TIME:
        oled.println(cflabel[CFG_CALIBRATION_TIME]);
        oled.set2X();
        oled.print(calibrationDelay / sec);

        /*
        Serial.print(cflabel[CFG_CALIBRATION_TIME]);
        Serial.print("\t");
        Serial.println(calibrationDelay);
        */
        break;
    
    }

    oled.set1X();

  }

  displayLastActivation = millis();
}

void setup() {
  Serial.begin(115200);

  startTime = millis();

  Serial.println(F("Begin Plant Watering"));

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

  // Buttons & IRQ
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);

  attachInterrupt(0, pin2IRQ, FALLING);

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
