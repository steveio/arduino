/**
 * Pin Change Interrupts
 * 
 * Sketch demonstrating setting Arduino Uno (ATMEGA328) pin change interupt registers
 * 
 * 1) Turn on Pin Change Interrupts
 * 2) Chose which pins to interrupt on
 * 3) Write an ISR for those pins
 * 
 * Includes state tracking to determine which pin(s) triggered 
 * 
 * Tested with a capacitive touch switch on pin D9 triggering wake from power save / sleep
 */

#include <avr/io.h>
#include <stdint.h>            // has to be added to use uint8_t
#include <avr/interrupt.h>    // Needed to use interrupts
#include <avr/sleep.h>

volatile uint8_t portbhistory = 0xFF;     // default is high because the pull-up


// ISR(PCINT0_vect){}    // Port B, PCINT0 - PCINT7
// ISR(PCINT1_vect){}    // Port C, PCINT8 - PCINT14
// ISR(PCINT2_vect){}    // Port D, PCINT16 - PCINT23

ISR (PCINT0_vect)
{
    sleep_disable();
    Serial.println("ISR PCINT0");

    uint8_t changedbits;

    changedbits = PINB ^ portbhistory;
    portbhistory = PINB;

    if(changedbits & (1 << PB0))
    {
      /* PCINT0 changed */
      Serial.println("PCINT0");
    }

    if(changedbits & (1 << PB1))
    {
      /* PCINT1 changed */
      Serial.print("PCINT1 ");
      if(PINB & (1 << PB1))
      {
        Serial.print("ON");
      } else {
        Serial.print("OFF");
      }
      Serial.println("");
    }


    if(changedbits & (1 << PB2))
    {
      /* PCINT2 changed */
      Serial.println("PCINT2");
    }
}

void setup() {

  Serial.begin(115200);

  cli();

  // 1 – Turn on Pin Change Interrupts
  PCICR |= 0b00000001;    // turn on port b (PCINT0 – PCINT7)
  // PCICR |= 0b00000010;    // turn on port c (PCINT8 – PCINT14)
  // PCICR |= 0b00000100;    // turn on port d (PCINT16 – PCINT23)
  // PCICR |= 0b00000111;    // turn on all ports

  // 2 – Choose Which Pins to Interrupt ( 3 mask registers correspond to 3 INT ports )
  PCMSK0 |= 0b00000111;    // turn on pins PB0, PB1, PB2, which is PCINT0, PCINT1, PCINT2 Chip physical pin 14,15,16, D8,D9,D10
  // PCMSK1 |= 0b00010000;    // turn on pin PC4, which is PCINT12, physical pin 27
  // PCMSK2 |= 0b10000001;    // turn on pins PD0 & PD7, PCINT16 & PCINT23

  // DDR (Port) - Data Direction Register  
  // PORTS- 
  // B = Digital  Pins 8-13
  // C = Analogue Pins 0-5 
  // D = Digital  Pins 0-7 

  DDRB &= ~((1 << DDB0) | (1 << DDB1) | (1 << DDB2)); // Clear the PB0, PB1, PB2 pin
  // PB0 (D8),PB1 (D9),PB2 (D10) (PCINT0, PCINT1, PCINT2 pin) are now inputs

  PORTB |= ((1 << PORTB0) | (1 << PORTB1) | (1 << PORTB2)); // turn On the Pull-up
  // PB0, PB1 and PB2 are now inputs with pull-up enabled

  //PCICR |= (1 << PCIE0);     // set (Pin Change Interrupt Counter) PCIE0 to enable PCMSK0 scan
  //PCMSK0 |= (1 << PCINT0);   // set (Pin Change Mask) PCINT0 to trigger an interrupt on state change 

  sei();                     // turn on interrupts

}

void loop() {

  uint8_t changedbits;

  changedbits = PINB;

  Serial.println((changedbits & (1 << PB0)));

  Serial.println((changedbits & (1 << PB1)));

  Serial.println((changedbits & (1 << PB2)));

  Serial.println("Sleep...");

  pinMode (LED_BUILTIN, OUTPUT);
  digitalWrite (LED_BUILTIN, HIGH);
  delay (50);
  digitalWrite (LED_BUILTIN, LOW);
  delay (50);
  pinMode (LED_BUILTIN, INPUT);
  
  // disable ADC
  ADCSRA = 0;  
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();

  // Do not interrupt before we go to sleep, or the
  // ISR will detach interrupts and we won't wake.
  noInterrupts ();
   
  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
  MCUCR = bit (BODS) | bit (BODSE);
  // The BODS bit is automatically cleared after three clock cycles
  MCUCR = bit (BODS); 
  
  // We are guaranteed that the sleep_cpu call will be done
  // as the processor executes the next instruction after
  // interrupts are turned on.
  interrupts ();  // one cycle
  sleep_cpu ();   // one cycle

}
