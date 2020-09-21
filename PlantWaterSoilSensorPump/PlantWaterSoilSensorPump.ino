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
const int AirValue = 1024;
const int WaterValue = 570;

const int soilSensorPin = A0;
unsigned long startTime = 0;
const unsigned long calibrationDelay = 30000; // time for sensor to calibrate (level)
int intervalDivider = 2; // 1-5   
int intervals = (AirValue - WaterValue)/intervalDivider;
int soilMoistureValue = 0;

int soilMoistureStatusId;

char l0[] = "V Wet";
char l1[] = "Wet";
char l2[] = "Dry";
char l3[] = "V Dry";

char * label[] = {l0, l1, l2, l3};

const int relayPin = 7;
bool pumpStatus = 0;
const unsigned long pumpActiveTime = 30000; // pump active duration
const unsigned long pumpDelay = 300000; // min time in ms between pump activations
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

bool displayActive = 0; // display active true/false
unsigned long displayTimeout = 300000; // delay to turn display off
unsigned long displayLastActivation = 0; // ts of last display activation

#define DISPLAY_STATUS 0x01


volatile bool irqStatus = 0;
volatile int activeButton = 0;

// buttons correspond to digital pin set bit in PORTD D3-D6
// PIND 0 (01), 1 (02), 2 (04), 3 b1 D3 (0x08), 4 b2 D4 (0x10), 5 b3 D5 (0x20), 6 b4 D6 (0x40)

#define BUTTON_01 0x08
#define BUTTON_02 0x10
#define BUTTON_03 0x20
#define BUTTON_04 0x40

unsigned long buttonDebounce = 150; // msecs
unsigned long buttonLastActive = 0;

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
    switch (activeButton)
    {
      case BUTTON_01:
        Serial.println("Button 1: ");
        break; 
      case BUTTON_02:
        Serial.println("Button 2: Pump on/off");
        (pumpStatus == 0) ? pumpOn() : pumpOff();
        break;
      case BUTTON_03:
        Serial.println("Button 3:");
        break;
      case BUTTON_04:
        Serial.println("Button 4:");
        break;
      default:
        break;
    }
  }

  activeButton = 0;
  buttonLastActive = millis();
  irqStatus = 0;
}

bool isCalibrating()
{
  return (millis() > startTime + calibrationDelay) ? false : true;
}

void pumpOn()
{
    pumpStatus = 1;
    digitalWrite(relayPin, LOW);  

    Serial.print(millis());
    Serial.print("\t");
    Serial.println("Pump On:");
}

void pumpOff()
{
    pumpStatus = 0;
    digitalWrite(relayPin, HIGH);

    Serial.print(millis());
    Serial.print("\t");
    Serial.println("Pump Off:");
}

// scheduled pump activation for duration pumpActiveTime
void activatePump()
{
  if(!isCalibrating() && (pumpLastActivation == 0 || millis() > pumpLastActivation + pumpDelay))
  {
    pumpOn();
    display(DISPLAY_STATUS);

    delay(pumpActiveTime);

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
    delay(1000);
  }
  soilMoistureValue = soilMoistureValue / 3;

  Serial.print(millis());
  Serial.print("\t");
  Serial.print(soilMoistureValue);
  Serial.print("\t");

  if(soilMoistureValue > WaterValue && (soilMoistureValue < (WaterValue + intervals))) // V Wet
  {
    soilMoistureStatusId = 0;
    Serial.println(label[soilMoistureStatusId]);
  }
  else if(soilMoistureValue > (WaterValue + intervals) && (soilMoistureValue < (AirValue - intervals))) // Wet
  {
    soilMoistureStatusId = 1;
    Serial.println(label[soilMoistureStatusId]);
  }
  else if(soilMoistureValue < AirValue && (soilMoistureValue > (AirValue - intervals))) // Dry
  {
    soilMoistureStatusId = 2;
    Serial.println(label[soilMoistureStatusId]);
    Serial.println("Activate pump: ");
    activatePump();
  }

  display(DISPLAY_STATUS);
}

void display(int opt)
{

  oled.setFont(Adafruit5x7);
  oled.clear();

  if (opt == DISPLAY_STATUS)
  {
    oled.println("Soil Moisture: ");
    oled.set2X();
    if (isCalibrating())
    {
      oled.println("Calibrating..");
    } else {
      oled.print(soilMoistureValue);
      oled.print(" ");
      oled.println(label[soilMoistureStatusId]);
    }
    oled.set1X();
    oled.println();
    oled.print("Pump:");
    oled.print(pumpStatus);
    oled.print(" / ");
    oled.println(pumpActivations);
  
    oled.print("Pump Last:");
    if (pumpLastActivation > 0)
    {
      oled.println((millis() - pumpLastActivation) / 1000);
    } else {
      oled.println(0);
    }
    oled.print("Dur/Delay:");
    oled.print(pumpActiveTime / 1000);
    oled.print(" / ");
    oled.println(pumpDelay / 1000);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Soil Moisture Sensor Test");

  delay(2000);

  // Init..
  startTime = millis();
  Serial.print("Start ts: ");
  Serial.println(startTime);

  Serial.print("Calibration Delay: ");
  Serial.println(calibrationDelay);

  Serial.print("Pump Duration: ");
  Serial.println(pumpActiveTime);

  Serial.print("Pump Delay: ");
  Serial.println(pumpDelay);

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
  readSoilMoisture();
  delay(200);
}
