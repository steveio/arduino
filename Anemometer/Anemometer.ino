/**
 * Anemometer - DIY Wind Speed Sensor
 * 
 * OPL550 IR Phototransistor sensor,  OPL240 IR led & optical light chopper interrupter
 * 
 * Phase = Time to complete a single rotation
 * Frequency = Number of rotations in a timed interval
 * RPM = Revolutions per minute
 * m/s = Metres per second
 * Linear Velocity (m/s) = v = 2π / 60 * r * N
 *    where: r = radius (metres), N = number of revolutions per minute
 * Kmph = (3 x π * radius * RPM / 25)
 * 
 * Variables to record: 
 * Sample Duration: 3 seconds / 20 samples per minute 
 * Every Minute: Compute Average / Max, store 60 minute totals
 * Every Hour: Compute Hourly Average / Max, store 3 hour totals
 * velocity (m/s) / max, 1 minute / 1 hour / 3 hour average & max (gust speed) 
 * 
 * Velocity (v) (metres / second):
 * uint16_t v[20];      // current velocity @ 3sec intervals, 20 per minute
 * uint16_t vAvgMin[60];  // minute avg velocity
 * uint16_t vAvgHour[3];  // hourly avg velocity 
 * 
 * Max (m)  (metres / second):
 * uint16_t m[20];      // 3sec intervals, 20 per minute
 * uint16_t mMin[60];  // velocity 
 * uint16_t mHour[3];
 *
 * Timestamp (secs since epoch)
 * uint16_t ts[20];      // 3sec intervals, 20 per minute
 * uint16_t mMin[60];  // velocity 
 * uint16_t mHour[3];
 *
 * Protype #1 to deploy 1hz AVR timer, then DS3231 RTC for timing   
 */

#include <avr/io.h>
#include <stdint.h>            // has to be added to use uint8_t
#include <avr/interrupt.h>    // Needed to use interrupts
#include <avr/sleep.h>

volatile long timerTicks = 0;   // 1hz timer ticks 
volatile long sensorTicks = 0;  // Optical IR sensor pulse counter
float ticksPerRev = 10;       // Pulses from optical light chopper in single revolution

#define SAMPLES_PER_MIN 20  // 3 second sample interval 
#define PI_CONSTANT 3.142
#define SENSOR_RADIUS 0.08  // Radius (mm) from axis to anemometer cup centre

volatile uint8_t portbhistory = 0xFF;


// 1hz Timer ISR
ISR(TIMER1_COMPA_vect) {
  timerTicks++;
}


// Optek Sensor Tick Counter - Pin Change Interrupt ISR
ISR (PCINT0_vect)
{
    uint8_t changedbits;
    changedbits = PINB ^ portbhistory;
    portbhistory = PINB;

    if(changedbits & (1 << PB1))
    {
      /* PCINT1 changed */
      // Serial.print("PCINT1 ");
      if(PINB & (1 << PB1))
      {
        // Serial.print("ON");
        sensorTicks++;
      } else {
        // Serial.print("OFF");
      }
      // Serial.println("");
    }

}

// Init 1hz timer
void setupTimer1() 
{
  noInterrupts();
  // Clear registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // 1 Hz (16000000/((15624+1)*1024))
  OCR1A = 15624;
  // CTC
  TCCR1B |= (1 << WGM12);
  // Prescaler 1024
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // Output Compare Match A Interrupt Enable
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

void setupPinChangeInterrupt()
{
  cli();

  PCICR |= 0b00000001;       // turn on Pin Change Ihterrupt port b (PCINT0 – PCINT7)
  PCMSK0 |= 0b00000010;    // turn on pin PB1 / PCINT1 / D9 
  DDRB &= ~((1 << DDB1)); // Clear PB1 pin
  PORTB |= ((1 << PORTB1)); // turn On the Pull-up

  sei();
}

void processSample()
{
  Serial.println("processSample()");

  sensorTicks = random(1, 300);

  Serial.print("SensorTicks: ");
  Serial.println(sensorTicks);

  float revs = sensorTicks / ticksPerRev;
  float rpm = revs * 20;

  Serial.print("Revolutions: ");
  Serial.println(revs);

  Serial.print("RPM: ");
  Serial.println(rpm);

  // convert to M/S 
  float velocity = rpmToLinearVelocity(rpm);

  Serial.print("Velocity (m/s): ");
  Serial.println(velocity);

  float kmph = rpmToKmph(rpm);
  Serial.print("Velocity (kmph): ");
  Serial.println(kmph);

  Serial.println("");
  
  sensorTicks = 0;
  revs = 0;
  rpm = 0;
}

float rpmToLinearVelocity(float rpm)
{
  return 2 * PI_CONSTANT / 60 * SENSOR_RADIUS * rpm;
}


float rpmToKmph(float rpm)
{
  return (3 * PI_CONSTANT * SENSOR_RADIUS * rpm / 25);
}

void setup() 
{

  Serial.begin(115200);

  setupTimer1();
  setupPinChangeInterrupt();

}

void loop() 
{

  if (timerTicks > 1 && timerTicks % 3 == 0) 
  {
    timerTicks = 0;
    processSample();
  }
}
