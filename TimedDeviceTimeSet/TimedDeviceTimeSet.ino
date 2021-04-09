/**
 * TimedDevice Time Display / Set
 * 
 * Demonstrates display & setting of time <hh>:<mm> on/off pair(s) 
 * using rotary encoder (w/ button), shift register & 4 Digit 7 Segment Display 
 * 
 */

// 4 Digit 7 Segment Display
int first_digit = 0;
int second_digit = 0;
int third_digit = 0;
int fourth_digit = 0;
int CA_1 = 12;
int CA_2 = 11;
int CA_3 = 10;
int CA_4 = 9;
int clk = 6;
int latch = 5;
int data = 4;
int digits[4] ;
int display_digits[4] ;
int CAS[4] = {12, 11, 10, 9};
//byte combinations for each number 0-9
byte numbers[10] {B11111100, B01100000, B11011010, B11110010, B01100110, B10110110, B10111110, B11100000, B11111110, B11110110};

const int LED_PIN = 13;
const int BUTTON_PIN = 7;

int ledState = 0;
int buttonState = 0;
unsigned long buttonLastEvent = 0;
unsigned long buttonDebounce = 500;

const int RE_CLK_PIN = 2;
const int RE_DT_PIN = 8;
const int RE_DT_SW = 3;

long reCounter = 0;
int reCounterIncrement = 1;
int reDirection;
int reStateCLK;
int reLastStateCLK;
unsigned long reLastEvent;
unsigned long reDebounce = 100;
int reButtonState = 0;
unsigned long reButtonLastEvent = 0;
unsigned long reButtonDebounce = 500;


// timer time elements
int t1OnHour = 0; // 0 - 24 h
int t1OnMinute = 0; // 0 - 59 m 
int t1OffHour = 0; // 0 - 24 h
int t1OffMinute = 0; // 0 - 59 m 

// pointer to timer time elements
int *t1OnHourPointer;
int *t1OnMinutePointer;
int *t1OffHourPointer;
int *t1OffMinutePointer;

#define CFG_ARRAY_SIZE 4

// indexed array of time element pointer
int cfgArray[CFG_ARRAY_SIZE];

// upper bounds cfg array time elements
uint8_t cfgArrayUpper[CFG_ARRAY_SIZE];

// pointer to selected config item (being editted)
int cfgArrayIdx = -1;



void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  buttonLastEvent = millis();

  pinMode(RE_CLK_PIN, INPUT);
  pinMode(RE_DT_PIN, INPUT);
  pinMode(RE_DT_SW, INPUT_PULLUP);
  reButtonLastEvent = millis();
  reLastStateCLK = digitalRead(RE_CLK_PIN);

  pinMode(CA_1, OUTPUT);
  pinMode(CA_2, OUTPUT);
  pinMode(CA_3, OUTPUT);
  pinMode(CA_4, OUTPUT);
  pinMode(clk, OUTPUT);
  pinMode(latch, OUTPUT);
  pinMode(data, OUTPUT);
  digitalWrite(CA_1, HIGH);
  digitalWrite(CA_2, HIGH);
  digitalWrite(CA_3, HIGH);
  digitalWrite(CA_4, HIGH);
  

  t1OnHourPointer = &t1OnHour;
  t1OnMinutePointer = &t1OnMinute;
  t1OffHourPointer = &t1OffHour;
  t1OffMinutePointer = &t1OffMinute;

  cfgArray[0] = t1OnHourPointer;
  cfgArray[1] = t1OnMinutePointer;
  cfgArray[2] = t1OffHourPointer;
  cfgArray[3] = t1OffMinutePointer;

  cfgArrayUpper[0] = 23;
  cfgArrayUpper[1] = 59;
  cfgArrayUpper[2] = 23;
  cfgArrayUpper[3] = 59;

}

void loop() {

  /*
   * PUSH BUTTON
   */
  int b = digitalRead(BUTTON_PIN);
  if (b != buttonState && millis() > buttonLastEvent + buttonDebounce)
  {
    buttonState = b;
    if (buttonState)
    {
      if (!ledState)
      {
        ledOn();
      } else {
         ledOff();
      }
    }
  }


  // ROTARY ENCODER
  reStateCLK = digitalRead(RE_CLK_PIN);
  if (reStateCLK != reLastStateCLK && reStateCLK == 1)
  {
    if (millis() > reLastEvent + reDebounce)
    {

      int * p = cfgArray[cfgArrayIdx];
      Serial.println("Current Val: ");
      Serial.println(*p);

      if ((digitalRead(RE_DT_PIN) != reStateCLK) && cfgArrayIdx != -1)
      {
        if (*p-1 > 0)
        {
          --*p;
        } else {
          *p = cfgArrayUpper[cfgArrayIdx];
        }
      } else {
        if (*p+1 <= cfgArrayUpper[cfgArrayIdx])
        {
          ++*p;
        } else {
          *p = 0;
        }
      }

      Serial.println("New Val: ");
      Serial.println(*p);

      if (cfgArrayIdx != -1)
      {
        Serial.print("Cfg Item: ");
        Serial.println(cfgArrayIdx);
        Serial.print("Value: ");
        int * n = cfgArray[cfgArrayIdx];
        Serial.println((int)*n);
      } else {
        Serial.println("No config item selected");
      }
    }
  }

  reLastStateCLK = reStateCLK;

  // ROTARY ENCODER BUTTON
  int b1 = digitalRead(RE_DT_SW);
  if (b1 == LOW)
  {
    if (millis() - reButtonLastEvent > reButtonDebounce)
    {

      Serial.println("Renc button click");

      Serial.print("CFG Array IDX:");
      if (cfgArrayIdx == -1)
      {
        Serial.print("NULL");
      } else {
        Serial.print(cfgArrayIdx);
      }
  
      if (cfgArrayIdx == -1)
      {
         cfgArrayIdx = 0;
      } else if (cfgArrayIdx < CFG_ARRAY_SIZE)
      {
        cfgArrayIdx++;  
      } else {
        cfgArrayIdx = -1;
      }

      Serial.print("CFG Array IDX (post):");
      if (cfgArrayIdx == -1)
      {
        Serial.print("NULL");
      } else {
        Serial.print(cfgArrayIdx);
      }
  
      reButtonLastEvent = millis();
    }
  }

  display_time();

}

void ledOn()
{
  ledState = 1;
  digitalWrite(LED_PIN, HIGH);
}

void ledOff()
{
  ledState = 0;
  digitalWrite(LED_PIN, LOW);
}

void break_number(long num) { // seperate the input number into 4 single digits

  first_digit = num / 1000;
  digits[0] = first_digit;
  int first_left = num - (first_digit * 1000);
  second_digit = first_left / 100;
  digits[1] = second_digit;
  int second_left = first_left - (second_digit * 100);
  third_digit = second_left / 10;
  digits[2] = third_digit;
  fourth_digit = second_left - (third_digit * 10);
  digits[3] = fourth_digit;
}


// Split selected (editable) timer elements
void display_time()
{

  if (cfgArrayIdx == -1)
  {
    return;
  }

  int * n;

  Serial.print("Display: ");

  if (cfgArrayIdx < 2) // Display Ontime #1: <t1OnHour> <t1OnMinute>
  {
    Serial.println("OnTime #1 ");

    n = cfgArray[0];
    break_number((long)*n);
    display_digits[0] = digits[2];
    display_digits[1] = digits[3];

    n = cfgArray[1];
    break_number((long)*n);
    display_digits[2] = digits[2];
    display_digits[3] = digits[3];

  } else if (cfgArrayIdx < 4)  //  Display Offtime #1:  <t1OffnHour> <t1OffMinute>
  {
    Serial.println("OffTime #1 ");

    n = cfgArray[2];
    break_number((long)*n);
    display_digits[0] = digits[2];
    display_digits[1] = digits[3];

    n = cfgArray[3];
    break_number((long)*n);
    display_digits[2] = digits[2];
    display_digits[3] = digits[3];

  }

  Serial.print(display_digits[0]);
  Serial.print(display_digits[1]);
  Serial.print(":");
  Serial.print(display_digits[2]);
  Serial.println(display_digits[3]);

  display_number();

}


void display_number()
{
  int count = 0;
  while(count < 4)
  {
    cathode_high(); //black screen
    digitalWrite(latch, LOW); //put the shift register to read
    shiftOut(data, clk, LSBFIRST, numbers[display_digits[count]]); //send the data
    digitalWrite(CAS[count], LOW); //turn on the relevent digit
    digitalWrite(latch, HIGH); //put the shift register to write mode
    count++;
    delay(1);
  }
}

//turn off all 4 digits
void cathode_high() 
{
  digitalWrite(CA_1, HIGH);
  digitalWrite(CA_2, HIGH);
  digitalWrite(CA_3, HIGH);
  digitalWrite(CA_4, HIGH);
}
