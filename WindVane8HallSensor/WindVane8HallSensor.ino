/**
 * Weather Vane - 8 Magnetic Hall Sensor Switch
 * 
 * A ring of 8 magnetic digital hall sensors 
 * attached to digital pins 3-10 are activated 
 * by a rotating magnet attached to vane shaft
 * 
 */


// current and previous active sensor pin
int active = NULL;
int lastActive = NULL;

// pin order direction labels
char d0[] = "S";
char d1[] = "W";
char d2[] = "SW";
char d3[] = "NW";
char d4[] = "N";
char d5[] = "E";
char d6[] = "SE";
char d7[] = "NE";

char * directionLabel[] = { d0, d1, d2, d3, d4, d5, d6, d7 };


volatile int irqState = 0;
unsigned long lastIrq;
int irqDelay = 100; // millisecs

/*
 * Pin Change Interrupts 
 * 
 */
void pin2IRQ()
{
  irqState = 1; 
}

ISR (PCINT0_vect)
{
  irqState = 1; 
}

ISR(PCINT2_vect)
{
  irqState = 1; 
}

void setupPinChangeInterrupt()
{
  cli();

  // 1 – Turn on Pin Change Interrupts
  PCICR |= 0b00000001;      // turn on port b (PCINT0 – PCINT7) pins D8 - D13
  PCICR |= 0b00000100;      // turn on port d (PCINT16 – PCINT23) pins D0 - D7

  // 2 – Choose Which Pins to Interrupt ( 3 mask registers correspond to 3 INT ports )
  PCMSK0 |= 0b00000111;    // turn on pins D8,D9,D10
  PCMSK2 |= 0b11111000;    // turn on pins D3 - D7 (PCINT19 - 23)

  sei();                     // turn on interrupts

}

void setup() {

  Serial.begin(115200);

  pinMode(4,INPUT_PULLUP);
  pinMode(5,INPUT_PULLUP);
  pinMode(6,INPUT_PULLUP);
  pinMode(7,INPUT_PULLUP);
  pinMode(8,INPUT_PULLUP);
  pinMode(9,INPUT_PULLUP);
  pinMode(10,INPUT_PULLUP);
  pinMode(11,INPUT_PULLUP);

  // interupt ISR per pin
  //setupPinChangeInterrupt();

  // common interrupt on pin d2
  attachInterrupt(0, pin2IRQ, FALLING);

  lastIrq = millis();

}

void loop() {

  int v;
  active = 0;

  if (irqState == 1 && (millis() - lastIrq > irqDelay))
  {

    for(int i = 3; i <= 10; i++)
    {
      v = digitalRead(i);

      if (v == 0)
      {
        active = i;
      }
    }
    if (active == 0) // magnet between sensor positions
    {
        active = lastActive;      
    }
    if (active != lastActive)
    {
      Serial.print(active);
      Serial.print("\t");
      Serial.println(directionLabel[active-3]);
    }

    lastActive = active;

    lastIrq = millis();
    irqState = 0;
  }

}
