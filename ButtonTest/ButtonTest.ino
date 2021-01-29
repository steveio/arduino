/**
 * Test Script for Arduino Push Buttons
 * 
 * 
 * @todo 
 *  - short/long/double press
 *  - eval button libs / refactor as class
 */

volatile bool irqStatus = 0;
volatile int activeButton = 0;

// buttons correspond to digital pin set bit in PORTD 
// PIND 0 (01), 1 (02), 2 (04), 3 b1 D3 (0x08), 4 b2 D4 (0x10), 5 b3 D5 (0x20), 6 b4 D6 (0x40)

#define BUTTON_01 0x08
#define BUTTON_02 0x10
#define BUTTON_03 0x20
#define BUTTON_04 0x40

void pin2IRQ()
{
  Serial.println("IRQ...");
  int x;
  x = PIND; // digital pins 0-7

  if ((x & BUTTON_01) == 0) {
    activeButton = BUTTON_01;
  }

  if ((x & BUTTON_02) == 0) {
    activeButton = BUTTON_02;
  }

  if ((x & BUTTON_03) == 0) {
    activeButton = BUTTON_03;
  }

  if ((x & BUTTON_04) == 0) {
    activeButton = BUTTON_04;
  }

  if (activeButton > 0)
  {
    irqStatus = 1;
  }
}

void handleButtonEvent()
{
  Serial.println(activeButton);

  switch (activeButton)
  {
    case BUTTON_01:
      Serial.println("Button 1 pressed");
      break; 
    case BUTTON_02:
      Serial.println("Button 2 pressed");
      break;
    case BUTTON_03:
      Serial.println("Button 3 pressed");
      break;
    case BUTTON_04:
      Serial.println("Button 4 pressed");
      break;
    default:
      break;
  }

  activeButton = 0;
  irqStatus = 0;
}



void setup() {

  Serial.begin(115200);

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);

  attachInterrupt(0, pin2IRQ, FALLING);
}

void loop() {

  if (irqStatus == 1)
  {
    handleButtonEvent();
  }

  int v = 0;

  v = digitalRead(2);
  Serial.println(v);

  v = digitalRead(3);
  Serial.println(v);

  v = digitalRead(4);
  Serial.println(v);

  v = digitalRead(5);
  Serial.println(v);

  v = digitalRead(6);
  Serial.println(v);

  
  Serial.println(" ");

  delay(200);
}
