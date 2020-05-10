/**
 * 433mhz Binary ASK encoded Radio Receiver
 *  
 * Demostrates Binary, Network Order Binary, ACII & Struct Messaging
 */

//#include "LowPower.h"
#include <avr/sleep.h>
#include <avr/wdt.h>

#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile

RH_ASK driver;

long counter = 0;

typedef struct
{
    float  humidity;
    float  tempC;
    float  tempF;
} SensorData;


void setup()
{
    Serial.begin(115200);  // Debugging only
    if (!driver.init())
         Serial.println("init failed");

  pinMode(LED_BUILTIN, OUTPUT);

}

void rxData()
{
    SensorData data;
    uint8_t datalen = sizeof(data);
    if (   driver.recv((uint8_t*)&data, &datalen)
        && datalen == sizeof(data))
    {
        float humidity = data.humidity;
        float tempC = data.tempC;
        float tempF = data.tempF;

        Serial.print("Message: ");
        Serial.println(counter);
        Serial.print(" ");

        Serial.print("Humidity ");
        Serial.print(humidity);
        Serial.print(",");
        Serial.print("TempC ");
        Serial.print(tempC);
        Serial.print(",");
        Serial.print("TempF ");
        Serial.println(tempF);

        counter++;
    }
}

// watchdog interrupt
ISR (WDT_vect)
{
  wdt_disable(); // disable watchdog
}

void goToSleep ()
{
  // disable ADC
  ADCSRA = 0;
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
}

void loop()
{
    rxData();

    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);                       // wait for a second

    //LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);

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
