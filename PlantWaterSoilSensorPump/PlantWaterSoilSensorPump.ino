/**
 * Automated Plant Watering
 * 
 * Arduino capacitive soil moisture sensor v1.2
 * 5v water pump attached via a relay
 * OLED SSD1306 124*64 display
 * 
 * When soil moisture level is found high (dry) pump is activated for n secs
 * Delays are implemented for sensor calibration and pump activations
 *
 */


// sensor calibration
const int AirValue = 970;
const int WaterValue = 550;

const int soilSensorPin = A0;
unsigned long startTime = 0;
const int calibrationDelay = 300000; // time for sensor to calibrate (level)
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
const int pumpActiveTime = 5000;
const int pumpDelay = 180000;
unsigned long pumpLastActivation = 0;

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
  return (millis() > startTime + calibrationDelay) ? 0 : 1;
}

void activatePump()
{
  if(isCalibrating() && (pumpLastActivation > millis() + pumpDelay))
  {
    Serial.print(millis());
    Serial.print("\t");
    Serial.println("Pump On:");
    pumpStatus = 1;
    displayStatus();

    digitalWrite(relayPin, HIGH);
    delay(pumpActiveTime);
    digitalWrite(relayPin, LOW);

    
    Serial.print(millis());
    Serial.print("\t");
    Serial.println("Pump Off:");

    pumpStatus = 0;
    displayStatus();

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

  if(soilMoistureValue > WaterValue && (soilMoistureValue < (WaterValue + intervals)))
  {
    soilMoistureStatusId = 0;
    Serial.println("Very Wet");
  }
  else if(soilMoistureValue > (WaterValue + intervals) && (soilMoistureValue < (AirValue - intervals)))
  {
    soilMoistureStatusId = 1;
    Serial.println("Wet");
  }
  else if(soilMoistureValue < AirValue && (soilMoistureValue > (AirValue - intervals)))
  {
    soilMoistureStatusId = 2;
    Serial.println("Dry");
    activatePump();
  }
  Serial.println("");

  displayStatus();
}

void displayStatus()
{

  oled.setFont(Adafruit5x7);

  oled.clear();
  oled.println("Soil Moisture: ");
  oled.set2X();
  oled.print(soilMoistureValue);
  oled.print(" ");
  oled.println(label[soilMoistureStatusId]);
  oled.set1X();
  oled.println();
  oled.print("Pump Status:");
  oled.println(pumpStatus);
  oled.print("Last Active:");
  if (pumpLastActivation > 0)
  {
    oled.println(millis() - pumpLastActivation);
  } else {
    oled.println(0);
  }
  oled.print("Calibrating:");
  oled.println(isCalibrating());

}

void setup() {
  Serial.begin(115200);
  Serial.println("Soil Moisture Sensor Test");

  startTime = millis();

  pinMode(soilSensorPin, INPUT);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

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
