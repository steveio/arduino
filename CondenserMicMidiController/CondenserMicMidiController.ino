/**
 * Electret Condenser Mic to Midi Generator
 *
 * Concept is to generate midi events from an audio signal
 *
 * Note:
 * Electret Mic circuit pre-amp is simple & sub-optimal
 *
 * See:
 *  https://circuitdigest.com/electronic-circuits/simple-led-music-light
 *  https://www.instructables.com/id/Send-and-Receive-MIDI-with-Arduino/
 *  https://electronics.stackexchange.com/questions/57683/can-i-use-a-pnp-transistor-with-an-electret-microphone-to-get-an-non-inverting-o
 *
 */

int analogPin = A0; // analogue pin to read

int analogVal;      // analog readings
float lastAnalogVal = 0;


float detectThreshold = 300; // high pass A/C filter
int detectCount = 0;
int analogDirection = 0;
int analogFallingMinHeight = 150;
int analogRisingMinHeight = 45;


// MIDI params
int midiNote = 48;
int velocity = 70;//velocity of MIDI notes, must be between 0 and 127
int noteON = 144;//144 = 10010000 in binary, note on command
int noteOFF = 128;//128 = 10000000 in binary, note off command


//send MIDI message
void MIDImessage(int command, int MIDInote, int MIDIvelocity) {
  Serial.write(command);//send note on or note off command
  Serial.write(MIDInote);//send pitch data
  Serial.write(MIDIvelocity);//send velocity data
}


void setup ()
{
  pinMode(analogPin, INPUT);
  Serial.begin(115200);

}

void loop ()
{

  // Read the analog interface
  analogVal = analogRead(analogPin);
  //Serial.print("AnalogVal:");
  //Serial.println(analogVal, DEC);

  // Simple Level (Falling) Triggered Event
  if ((analogVal < detectThreshold) && (analogVal < lastAnalogVal) && ((lastAnalogVal- analogVal) > analogFallingMinHeight))
  {
      //Serial.println("FALLING - Note OFF");
      analogDirection = 1;
      MIDImessage(noteON, midiNote, velocity); // send a midi note on event
  } else {
    if (analogDirection == 1 && ((analogVal - lastAnalogVal) > analogRisingMinHeight))
    {
      //Serial.println("RISING - Note OFF");
      analogDirection = 0;
      MIDImessage(noteOFF, midiNote, velocity); // send a midi not off event
    }
  }

  lastAnalogVal = analogVal;

  delay(75);

}
