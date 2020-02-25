/**
 * 
 * Arduino UltraSonic HC-SR04
 * 
 * @range  2cm - 400cm 
 * @accuracy ~3mm
 * Test distance = (high level time×velocity of sound (340M/S) / 2
 * 
 * Notes - Object detection varies according to size (and reflectivity/material?) of object
 * Experientially, smaller objects (a human hand for example) detect reliably in ranges upto ~60cm
 * Larger objects (a 50cmx50cm) cardboard box have a longer detection range.
 * 
 * 
 */

#include "LedControl.h"


LedControl lc=LedControl(10,12,11,1);


// defines pins numbers
const int trigPin = 7;
const int echoPin = 8;

// defines variables
long duration;

int distance;
int max_range = 60; // ~(60 - 100) 
int level = 0;

unsigned long obj_detected;
unsigned long delaytime=3000;

byte l[8]= {0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF};


void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(115200); // Starts the serial communication

  lc.shutdown(0,false);
  lc.setIntensity(0,6);
  lc.clearDisplay(0);  

}

void loop() {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  //distance= duration*0.034/2;
  // cm = (duration/2) / 29.1;
  distance= duration/52.8;

  Serial.print("Duration: ");
  Serial.println(duration);

  Serial.print("Distance: ");
  Serial.println(distance);

  level = 0;

  if (distance < max_range)
  {
    obj_detected = millis();
    int i = (max_range - distance);
    level = map(i,0,max_range,1,9);
    Serial.print("Level: ");
    Serial.println(level);

    if (level > 0 && level <= 8)
    {
        lc.setColumn(0,0,l[level-1]);
    }
  } else { // clear display if n millis since last object detected
      if (millis() - obj_detected > delaytime)
      {
        lc.clearDisplay(0);
      }
  }

  delayMicroseconds(20000); // ~ (25 - 30ms) to ensure return pulse read time
}
