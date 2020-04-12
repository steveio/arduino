/**
 * Arduino Audio Analyser - Amplitude (Level), Frequency
 * 
 * Code from:
 * https://blog.yavilevich.com/2016/08/arduino-sound-level-meter-and-spectrum-analyzer/
 * 
 * Demonstrates: 
 *  - fast high frequency analogue sampling
 *  - fourier discreet hartley transform
 *  
 *  Tested with Keyes KY-037 Microphone
 */

#define MicSamples (1024*2)
#define MicPin A0
#define AmpMax (1024 / 2)
#define VolumeGainFactorBits 0


// FHT, http://wiki.openmusiclabs.com/wiki/ArduinoFHT
#define LOG_OUT 1 // use the log output function
#define LIN_OUT8 1 // use the linear byte output function
#define FHT_N 16 // set to 256 point fht
#include <FHT.h> // include the library

 
// modes
//#define Use3.3 // use 3.3 voltage. the 5v voltage from usb is not regulated. this is much more stable.
#define ADCReClock // switch to higher clock, not needed if we are ok with freq between 0 and 4Khz.
#define ADCFlow // read data from adc with free-run (not interupt). much better data, dc low. hardcoded for A0.

#define FreqLog // use log scale for FHT frequencies
#ifdef FreqLog
#define FreqOutData fht_log_out
#define FreqGainFactorBits 0
#else
#define FreqOutData fht_lin_out8
#define FreqGainFactorBits 3
#endif
#define FreqSerialBinary


// macros
// http://yaab-arduino.blogspot.co.il/2015/02/fast-sampling-from-analog-input.html
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

void setup ()
{
  pinMode(MicPin, INPUT); 
  Serial.begin(115200);

  analogReference(EXTERNAL); // 3.3V to AREF
  
  // register explanation: http://maxembedded.com/2011/06/the-adc-of-the-avr/
  // 7 =&gt; switch to divider=128, default 9.6khz sampling
  ADCSRA = 0xe0+7; // "ADC Enable", "ADC Start Conversion", "ADC Auto Trigger Enable" and divider.
  ADMUX = 0x0; // Use adc0 (hardcoded, doesn't use MicPin). Use ARef pin for analog reference (same as analogReference(EXTERNAL)).
  #ifndef Use3.3
  ADMUX |= 0x40; // Use Vcc for analog reference.
  #endif
  DIDR0 = 0x01; // turn off the digital input for adc0
}

void loop ()
{
  //MeasureAnalog();
  //MeasureAnalogFreeRunning();
  //MeasureVolume();
  MeasureFHT();
}

void MeasureFHT()
{
    long t0 = micros();
    for (int i = 0; i < FHT_N; i++) { // save 256 samples
        while (!(ADCSRA & /*0x10*/_BV(ADIF))); // wait for adc to be ready (ADIF)
        sbi(ADCSRA, ADIF); // restart adc
        byte m = ADCL; // fetch adc data
        byte j = ADCH;
        int k = ((int)j << 8) | m; // form into an int
        k -= 0x0200; // form into a signed int
        k <<= 6; // form into a 16b signed int
        fht_input[i] = k; // put real data into bins
    }
    long dt = micros() - t0;
    fht_window(); // window the data for better frequency response
    fht_reorder(); // reorder the data before doing the fht
    fht_run(); // process the data in the fht
    fht_mag_log();


    // print as text
    for (int i = 0; i < FHT_N / 2; i++)
    {
        Serial.print(FreqOutData[i]);
        Serial.print(',');
    }
    //Serial.print(dt);
    //Serial.print(',');

    long sample_rate = FHT_N * 1000000l / dt;
    Serial.println(" ");
}

void MeasureAnalogFreeRunning()
{
  for (int i = 0; i < MicSamples; i++)
  {
      while (!(ADCSRA & /*0x10*/_BV(ADIF))); // wait for adc to be ready (ADIF)
      sbi(ADCSRA, ADIF); // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = ((int)j << 8) | m; // form into an int
      // work with k
      Serial.println(k);
  }
}

// calculate volume level of the signal and print to serial and LCD
void MeasureVolume()
{
    long soundVolAvg = 0, soundVolMax = 0, soundVolRMS = 0, t0 = millis();
    for (int i = 0; i < MicSamples; i++)
    {
#ifdef ADCFlow
        while (!(ADCSRA & /*0x10*/_BV(ADIF))); // wait for adc to be ready (ADIF)
        sbi(ADCSRA, ADIF); // restart adc
        byte m = ADCL; // fetch adc data
        byte j = ADCH;
        int k = ((int)j << 8) | m; // form into an int
#else
        int k = analogRead(MicPin);
#endif
        int amp = abs(k - AmpMax);
        amp <<= VolumeGainFactorBits;
        soundVolMax = max(soundVolMax, amp);
        soundVolAvg += amp;
        soundVolRMS += ((long)amp*amp);
    }
    soundVolAvg /= MicSamples;
    soundVolRMS /= MicSamples;
    float soundVolRMSflt = sqrt(soundVolRMS);
    float dB = 20.0*log10(soundVolRMSflt/AmpMax);
 
    // convert from 0 to 100
    soundVolAvg = 100 * soundVolAvg / AmpMax; 
    soundVolMax = 100 * soundVolMax / AmpMax; 
    soundVolRMSflt = 100 * soundVolRMSflt / AmpMax;
    soundVolRMS = 10 * soundVolRMSflt / 7; // RMS to estimate peak (RMS is 0.7 of the peak in sin)
 
    // print
    //Serial.print("Time: " + String(millis() - t0));
    Serial.print(" Amp: Max: " + String(soundVolMax));
    Serial.print("% Avg: " + String(soundVolAvg));
    Serial.print("% RMS: " + String(soundVolRMS));
    Serial.println("% dB: " + String(dB,3));
}

// measure basic properties of the input signal
// determine if analog or digital, determine range and average.
void MeasureAnalog()
{
    long signalAvg = 0, signalMax = 0, signalMin = 1024, t0 = millis();
    for (int i = 0; i < MicSamples; i++)
    {
        int k = analogRead(MicPin);
        signalMin = min(signalMin, k);
        signalMax = max(signalMax, k);
        signalAvg += k;
    }
    signalAvg /= MicSamples;
 
    // print
    //Serial.print("Time: " + String(millis() - t0));
    Serial.print(" Min: " + String(signalMin));
    Serial.print(" Max: " + String(signalMax));
    Serial.print(" Avg: " + String(signalAvg));
    Serial.print(" Span: " + String(signalMax - signalMin)); // distance min/max
    Serial.print(", " + String(signalMax - signalAvg)); // distance max/avg
    Serial.print(", " + String(signalAvg - signalMin)); // distance min/avg
    Serial.println("");
}
