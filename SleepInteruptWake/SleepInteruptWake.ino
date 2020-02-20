/**
 * SleepInteruptWake
 * 
 * A test sketch to explore Arduino sleep / low power consumption mode / wake functions
 * 
 */

#include <avr/sleep.h>

const int ledPinRed = 12;
const int ledPinGreen = 13;
const int buttonPin = 2;

volatile int deviceState = 0;
volatile int interuptState = 0;

// push button appears to cause interupt to fire multiple times, though state change is specified as HIGH
// From the docs:
// Pushbuttons often generate spurious open/close transitions when pressed, due to mechanical and physical issues
// https://www.arduino.cc/en/Tutorial/Debounce
volatile unsigned long lastInterruptMillis;
volatile unsigned long currentMillis;
const unsigned long interruptWaitPeriod = 1000;

void setup() 
{

  Serial.begin(19200);
  
  pinMode(ledPinRed,OUTPUT);
  pinMode(ledPinGreen,OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(buttonPin), handleInterupt, HIGH);

}

void loop() 
{
  Serial.println("loop");
  Serial.println(deviceState);
  Serial.println(interuptState);

  delay(3000);

  setDeviceState();
}

void handleInterupt()
{
  currentMillis = millis();
  if ((currentMillis - lastInterruptMillis) > interruptWaitPeriod) 
  {
    deviceState ^= 1 << 0;
    interuptState = 1;
  }
  lastInterruptMillis = millis();
  
}

void setDeviceState()
{ 

  if (interuptState == 1) // only change device status arriving from interupt
  {
    interuptState = 0;
    (deviceState == 1) ? activateSleep() : activateWakeUp();
  }
}

void activateSleep()
{
  Serial.println("activateSleep()");
  digitalWrite(ledPinRed,HIGH);
  delay(3000);
  digitalWrite(ledPinRed,LOW);
  delay(1000);

  /*
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_cpu();
  */
}

void activateWakeUp()
{
  //sleep_disable();
  Serial.println("activateWakeUp()");
  digitalWrite(ledPinGreen,HIGH);
  delay(3000);
  digitalWrite(ledPinGreen,LOW);
  delay(1000);

  //detachInterrupt(digitalPinToInterrupt(buttonPin));
}
