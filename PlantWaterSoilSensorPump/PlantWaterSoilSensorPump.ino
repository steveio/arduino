/**
 * Automated Plant Watering
 * 
 * Arduino Pro Mini 3.3v 
 * Capacitive soil moisture sensor v1.2
 * 5v water pump attached via a relay (wired normally closed)
 * OLED SSD1306 124*64 display
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
int intervals = (AirValue - WaterValue)/3;
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


bool isCalibrating()
{
  return (millis() > startTime + calibrationDelay) ? false : true;
}

void activatePump()
{
  if(!isCalibrating() && (pumpLastActivation == 0 || millis() > pumpLastActivation + pumpDelay))
  {
    Serial.print(millis());
    Serial.print("\t");
    Serial.println("Pump On:");

    pumpStatus = 1;
    displayStatus();

    digitalWrite(relayPin, LOW);
    delay(pumpActiveTime);
    digitalWrite(relayPin, HIGH);

    Serial.print(millis());
    Serial.print("\t");
    Serial.println("Pump Off:");

    pumpStatus = 0;
    displayStatus();

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

  displayStatus();
}

void displayStatus()
{

  oled.setFont(Adafruit5x7);

  oled.clear();
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

void setup() {
  Serial.begin(115200);
  Serial.println("Soil Moisture Sensor Test");

  delay(2000);

  startTime = millis();
  Serial.print("Start ts: ");
  Serial.println(startTime);

  Serial.print("Calibration Delay: ");
  Serial.println(calibrationDelay);

  Serial.print("Pump Duration: ");
  Serial.println(pumpActiveTime);

  Serial.print("Pump Delay: ");
  Serial.println(pumpDelay);

  pinMode(soilSensorPin, INPUT);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // switch pump off

  Wire.begin();
  Wire.setClock(400000L);

  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

}

void loop() {
  readSoilMoisture();
  delay(100);
}
