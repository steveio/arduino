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
 * Variables to record: velocity (m/s) / max velocity (m/s), minute / hour / 3 hour average & max
 * Sample Duration / Interval: 3 seconds / 20 samples per minute 
 * Every Minute: Compute Average / Max, store 60 minute totals
 * Every Hour: Compute Hourly Average / Max, store 3 hour totals
 *
 * Data structures:
 * Velocity v (metres / second):
 * uint16_t v[20];        // current velocity @ 3sec intervals, 20 per minute
 * uint16_t vAvgMin[60];  // minute avg velocity
 * uint16_t vAvgHour[3];  // hourly avg velocity 
 * 
 * Protype deploy 1hz AVR timer, then DS3232 RTC for timing
 *
 * Timestamp (secs since epoch)
 * uint16_t ts[20];      // 3sec intervals, 20 per minute
 * uint16_t mMin[60];  // velocity 
 * uint16_t mHour[3];
 *
 */

#include <avr/io.h>
#include <stdint.h>            // has to be added to use uint8_t
#include <avr/interrupt.h>    // Needed to use interrupts

volatile long timerTicks = 0;   // 1hz timer ticks 
volatile long sensorTicks = 0;  // Optical IR sensor pulse counter

#define SAMPLE_FREQ_MIN 20      // 3 second sample interval 
#define SAMPLE_FREQ_HOUR 60     // minute average / max 
#define SAMPLE_FREQ_DAY 24      // hourly average / max 
#define PI_CONSTANT 3.142
#define SENSOR_RADIUS 0.08  // Radius (mm) from axis to anemometer cup centre
#define SENSOR_TICKS_REV 10 // Optical Sensor Pulses in single revolution

float velocity, maxVelocity;
int secIndex, minIndex, hourIndex = 0;

// Velocity - Average / Max (m/s)
float v[SAMPLE_FREQ_MIN];         // current velocity @ 3sec intervals, 20 samples per minute
float vAvgMin[SAMPLE_FREQ_HOUR];  // 60 minute avg velocity
float vAvgHour[SAMPLE_FREQ_DAY];  // 24 hour avg velocity 
float vMaxMin[SAMPLE_FREQ_HOUR];  // 60 minute max velocity
float vMaxHour[SAMPLE_FREQ_DAY];  // 24 hour max velocity 

int hourSimMax = 100; // Simulated Max RPM 

volatile uint8_t portbhistory = 0xFF;


// Timer ISR
ISR(TIMER1_COMPA_vect) {
  timerTicks++;
}


// Optical Sensor Tick Counter - Pin Change Interrupt ISR
ISR (PCINT0_vect)
{
    uint8_t changedbits;
    changedbits = PINB ^ portbhistory;
    portbhistory = PINB;

    if(changedbits & (1 << PB1))
    {
      /* PCINT1 changed */
      if(PINB & (1 << PB1))
      {
        sensorTicks++;
      } else {
        // Serial.print("OFF");
      }
    }

}

void clearTimer()
{
  cli();
  TIMSK1 = 0;
  sei();
}

// Init Timer
void setupTimer1() 
{
  cli();
  // Clear registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // 1 Hz (16000000/((15624+1)*1024))
  //OCR1A = 15624;

  // Higher timer rates increase simulation speed
  // 5 Hz (16000000/((3124+1)*1024))
  //OCR1A = 3124;
  // 25 Hz (16000000/((624+1)*1024))
  //OCR1A = 624;

  // 500 Hz (16000000/((124+1)*256))
  OCR1A = 124;
  
  // CTC
  TCCR1B |= (1 << WGM12);

  // Prescaler 1024 (1/5/25 Hz)
  //TCCR1B |= (1 << CS12) | (1 << CS10);

  // Prescaler 256 (500hz)
  TCCR1B |= (1 << CS12);
  
  // Output Compare Match A Interrupt Enable
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

// ISR event trigger for sensor pulse
void setupPinChangeInterrupt()
{
  cli();

  PCICR |= 0b00000001;       // turn on Pin Change Ihterrupt port b (PCINT0 – PCINT7)
  PCMSK0 |= 0b00000010;    // turn on pin PB1 / PCINT1 / D9 
  DDRB &= ~((1 << DDB1)); // Clear PB1 pin
  PORTB |= ((1 << PORTB1)); // turn On the Pull-up

  sei();
}

// Periodic Wind Speed Calculator
void processSample()
{
  sensorTicks = random(1, hourSimMax); // simulation

  float revs = sensorTicks / (float) SENSOR_TICKS_REV;
  float rpm = revs * SAMPLE_FREQ_MIN;

  // calculate linear velocity (metres per second) 
  velocity = rpmToLinearVelocity(rpm);

  //Serial.print("Revolutions: ");
  //Serial.println(revs);

  //Serial.print("RPM: ");
  //Serial.println(rpm);

  //Serial.print("Velocity (m/s): ");
  //Serial.println(velocity);

  if (velocity > maxVelocity) // Update 1 minute max velocity
  {
    maxVelocity = velocity;
  }

  // Todo - Conversions - Kmph/Mph/Beaufort Scale
  //float kmph = rpmToKmph(rpm);
  //Serial.print("Velocity (kmph): ");
  //Serial.println(kmph);

  // record sample to 1 minute velocity array
  v[secIndex++] = velocity;

  updateStats();
  
  sensorTicks = 0;
  revs = 0;
  rpm = 0;
}

// Compute Minute, Hour and Daily Stats = Avg / Max
void updateStats()
{
  if (secIndex == SAMPLE_FREQ_MIN)
  {
    float avg = computeAvg(v, SAMPLE_FREQ_MIN);

    vAvgMin[minIndex] = avg;
    vMaxMin[minIndex] = maxVelocity;

    maxVelocity = 0;
    minIndex++;

    if (minIndex == SAMPLE_FREQ_HOUR)
    {
      float avg = computeAvg(vAvgMin, SAMPLE_FREQ_HOUR);
      float mx = computeMax(vMaxMin, SAMPLE_FREQ_HOUR);

      vAvgHour[hourIndex] = avg;
      vMaxHour[hourIndex] = mx;
      hourIndex++;

      printStats();
      hourSimMax = random(0,random(0,700)); // generate a new simulated hourly max RPM

      if (hourIndex == SAMPLE_FREQ_DAY)
      {
          printStats();

          //clearTimer(); // stop simulation end of 24h period

          hourIndex = 0; // reset day counter
      }

      minIndex = 0; // reset hour counter
    }

    secIndex = 0;
  }

}

void printStats()
{
  float avg = computeAvg(vAvgHour, SAMPLE_FREQ_DAY);
  float mx = computeMax(vMaxHour, SAMPLE_FREQ_DAY);

  Serial.println("24h Wind Speed Stats: ");
  
  Serial.print("1 day Average: ");
  Serial.println(avg);
    
  Serial.print("1 day Max: ");
  Serial.println(mx);

  Serial.println("Previous Hour: ");

  Serial.println("Minute Average: ");

  for(int i = 0; i<SAMPLE_FREQ_HOUR; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(vAvgMin[i]);
  }

  Serial.println("Minute Max: ");

  for(int i = 0; i<SAMPLE_FREQ_HOUR; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(vMaxMin[i]);

  }

  Serial.println("Hourly Average: ");

  for(int i = 0; i<SAMPLE_FREQ_DAY; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(vAvgHour[i]);
  }

  Serial.println("Hourly Max: ");

  for(int i = 0; i<SAMPLE_FREQ_DAY; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(vMaxHour[i]);

  }

}

// compute simple average from array of values
float computeAvg(float v[], uint8_t freq)
{
  // compute average
  float sum = 0, avg;
  for(int i = 0; i<freq; i++)
  {
    sum += v[i];
  }
  return avg = sum / freq;
}

// find max from array of values
float computeMax(float v[], uint8_t freq)
{
  float mx = 0;
  for(int i = 0; i<freq; i++)
  {
    mx = (v[i] > mx) ? v[i] : mx;
  }
  return mx;
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

  // calculate wind speed at 3 second intervals
  if (timerTicks > 1 && timerTicks % 3 == 0) 
  {
    timerTicks = 0;
    processSample();
  }

}
