/**
 * Low Power Mode Sleep Test
 *  
 * Sketch to measure current consumption in various operating modes. 
 * 
 * Compare Low Power Libary ( https://github.com/rocketscream/Low-Power ) 
 * with manual sleep WDT interrupt wake 
 * 
 * Multimeter current readings are influenced by 
 *  - power source 2x 3.7v Li-ion batteries in parrallel with a 5v Boost Converter
 *  - (Boost converter appears to draw current even when Arduino is sleeping)
 *  - 433mhz RF Receiver attached to Arduino 
 * 
 */


//#include "LowPower.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>


// watchdog interrupt
ISR (WDT_vect)
{
  wdt_disable(); // disable watchdog
}

void goToSleep ()
{
  // disable ADC
  ADCSRA = 0;

  power_all_disable();
  
  // clear various "reset" flags
  MCUSR = 0;
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0); // set WDIE, and 8 seconds delay
  wdt_reset(); // reset the watchdog
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  noInterrupts (); // timed sequence follows
  sleep_enable();
  // turn off brown‐out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);
  interrupts (); // guarantees next instruction executed
  sleep_cpu ();
  // cancel sleep as a precaution
  sleep_disable();

  power_all_enable();

}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {

  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second

  // Normal Current Consumption (including an 433mhz RF receiver) ~80 mA

  // powerDown Result ~ 55mA
  //LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

  // idle Result ~ 65mA
  //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);

  // Manual Result ~ 55mA
  goToSleep();

  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(300);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(300);                       // wait for a second
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(300);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(300);                       // wait for a second

  delay(1000);                       // wait for a second

}
