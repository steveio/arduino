/**
 * WatchDog timer interupts sleep after approx 8 secs (1024k osc cycles)
 * Examples: setting bit register values
 */

#include <avr/sleep.h>
#include <avr/wdt.h>

const byte LED = 12;

void flash ()
  {
  pinMode (LED, OUTPUT);
  for (byte i = 0; i < 10; i++)
    {
    digitalWrite (LED, HIGH);
    delay (50);
    digitalWrite (LED, LOW);
    delay (50);
    }
    
  pinMode (LED, INPUT);
    
  }  // end of flash
  
// Watchdog interrupt handler
// Errors in handler (use of delay(), or Serial.println()) 
// may cause microcontroller to hang preventing watchdog from restarting
ISR (WDT_vect) 
{
   wdt_disable();  // disable watchdog
}  // end of WDT_vect
 
void setup () { }

void loop () 
{
 
  flash ();
  
  // disable ADC
  ADCSRA = 0;  

  // clear various "reset" flags
  MCUSR = 0;
  // (WD)TCRS WatchDog Timer Control Register      
  // Change Enable, System Reset (disable) 
  WDTCSR = bit (WDCE) | bit (WDE);   
  // set WDIE (Interrupt Enable), and 8 seconds delay (at VCC = 5v)
  // WD-P3,P2,P1,P0  1001 - 1024k WDT Oscillator cycles
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    
  wdt_reset();  // pat the dog

  // Further Example: Enter Watchdog Configuration mode:
  //WDTCSR |= (1<<WDCE) | (1<<WDE);
  // Set Watchdog settings:
  //WDTCSR = (1<<WDIE) | (1<<WDE) | (0<<WDP3) | (1<<WDP2) | (1<<WDP1) | (0<<WDP0);

  // Using binary: Enter config mode
  WDTCSR |= B00011000;
  // Set Watchdog settings:
  WDTCSR = B01001110;

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  noInterrupts ();           // timed sequence follows
  sleep_enable();
 
  // turn off brown-out enable in software
  //MCUCR = bit (BODS) | bit (BODSE);
  //MCUCR = bit (BODS); 
  interrupts ();             // guarantees next instruction executed
  sleep_cpu ();  
  
  // cancel sleep as a precaution
  sleep_disable();
  
  } // end of loop
