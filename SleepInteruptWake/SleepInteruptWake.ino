/**
 * SleepInteruptWake
 * 
 * Explore Arduino sleep / wake and low power consumption mode(s)
 * 
 * Writtem for Arduino Mega 2560 
 */

#include <avr/sleep.h>
#include <avr/power.h>


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
  Serial.println("loop()");
  delay(3000);

  // sleep/wake cycle
  //sleepTest();

  // interupt based
  setDeviceState();
}

void handleInterupt()
{
  Serial.println("Interupt");

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
    (deviceState == 1) ? sleep() : wakeUp();
  }
}

/**
 * For Arduino Mega 2560 see DataSheet
 * https://ww1.microchip.com/downloads/en/devicedoc/atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf
 * 11. Power Management and Sleep Modes
 * 
 */
void sleepTest()
{

  Serial.println("Sleep");
  digitalWrite(ledPinRed,HIGH);
  delay(3000);
  digitalWrite(ledPinRed,LOW);
  delay(1000);
  cli (); // disable interrupts

  enableLowPowerState();
 
  // sleep_bod_disable (); // not declared

  /**
   * Sleep modes and power consumption:

      SLEEP_MODE_IDLE: 15 mA
      SLEEP_MODE_ADC: 6.5 mA
      SLEEP_MODE_PWR_SAVE: 1.62 mA
      SLEEP_MODE_EXT_STANDBY: 1.62 mA
      SLEEP_MODE_STANDBY : 0.84 mA
      SLEEP_MODE_PWR_DOWN : 0.36 mA

   */
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();
  sei (); // enable interrupts

  //sleep_cpu (); // Mega 2560 does not wake if this is set?


  sleep_disable();
  disableLowPowerState();

  Serial.println("WakeUp");
  digitalWrite(ledPinGreen,HIGH);
  delay(3000);
  digitalWrite(ledPinGreen,LOW);
  delay(1000);

}

void sleep()
{
  Serial.println("sleep()");
  digitalWrite(ledPinRed,HIGH);
  delay(1000);
  digitalWrite(ledPinRed,LOW);

  cli (); // disable interrupts
  enableLowPowerState(); 
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();
  sei (); // enable interrupts

}

void wakeUp()
{
  sleep_disable();
  disableLowPowerState();

  Serial.println("WakeUp");
  digitalWrite(ledPinGreen,HIGH);
  delay(1000);
  digitalWrite(ledPinGreen,LOW);

}

void enableLowPowerState()
{
  
  // set all pins to LOW
  for (int i = 0; i <= 53; i++) {
    //digitalWrite(i, LOW);
  }

  power_adc_disable();
  power_spi_disable();
  power_usart0_disable();
  power_usart1_disable();
  power_usart2_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_timer3_disable();
  power_timer4_disable();
  power_timer5_disable();
  power_twi_disable();

}


void disableLowPowerState()
{
  digitalWrite(13, LOW); // enable internal LED
 
  power_adc_enable();
  power_spi_enable();
  power_usart0_enable();
  power_usart1_enable();
  power_usart2_enable();
  power_timer1_enable();
  power_timer2_enable();
  power_timer3_enable();
  power_timer4_enable();
  power_timer5_enable();
  power_twi_enable();

}
