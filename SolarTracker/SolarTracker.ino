/**
 * Dual Axis Solar Tracker
 * 
 * 4x LDR (GL5537) arranged in + configuration 
 * Interfaced to CD4051be Analogue Multiplexer
 * 
 * References:
 * http://tok.hakynda.com/article/detail/144/cd4051be-cmos-analog-multiplexersdemultiplexers-with-logic-level-conversion-analog-input
 * https://theiotprojects.com/dual-axis-solar-tracker-arduino-project-using-ldr-servo-motors/
 * 
 */

// OLED LCD
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C
#define RST_PIN -1
SSD1306AsciiWire oled;


// Joystick
int analogueHigh = 1024;
const int xPin = A1;
const int yPin = A2;

int xPos = 512;
int xCentre = 512;
int xLimitLow = 200;
int xLimitHigh = 1024;
int xIncrement = 10;

int yPos = 512;
int yCentre = 512;
int yLimitLow = 200;
int yLimitHigh = 1024;
int yIncrement = 10;

// horizontal (pan) stepper motor
#include <Stepper.h>
const int stepsPerRev = 2048;
const int stepsPerHalfRev = 1024;
Stepper Stepper1 = Stepper(stepsPerRev, 8, 10, 9, 11);

// vertical (tilt) servo
#include <Servo.h>
int servoPin = 7;
Servo Servo1;


const int selectPins[3] = {2, 3, 4}; // S0~2, S1~3, S2~4
const int zInput = A0; // Connect common (Z) to A0 (analog input)

// Analog Voltage Meter
#define NUM_SAMPLES 10

int sum = 0;
unsigned char sample_count = 0;
float voltage = 0.0;

int prPin = A0; 
int prVal = 0;



/*
 * Test Servo Tilt
 */
void tiltTest()
{
  Serial.println("Servo: 0 degrees"); 
  Servo1.write(0);
  delay(3000); 

  Serial.println("Servo: 45 degrees");
  Servo1.write(45); 
  delay(3000); 

  Serial.println("Servo: 90 degrees");
  Servo1.write(90); 
  delay(3000); 

}

// Test Stepper Pan (Rotate)
void panTest()
{
  Serial.println("clockwise");
  Stepper1.step(stepsPerRev / 2);
  delay(500);
  
  // Step one revolution in the other direction:
  Serial.println("counterclockwise");
  Stepper1.step(-stepsPerRev /2);
  delay(500);
}

void track()
{
  // LDR sensor array + config -> multiplexer IO (channels)
  // 4 7
  // 6 5

  // To read CD4051B channels 0-7:
  //
  // Channel 4 = 0100
  // digitalWrite(2, 0);
  // digitalWrite(3, 0);
  // digitalWrite(4, 1);
  //
  // Or using direct port manipulation
  //  PORTD = B00010000;


  PORTD = B00010000;
  int lt = analogRead(zInput); // top left

  PORTD = B00011100;
  int rt = analogRead(zInput); // top right

  PORTD = B00011000;
  int ld = analogRead(zInput); // down left

  PORTD = B00010100;
  int rd = analogRead(zInput); // down right

  Serial.print(lt);
  Serial.print("\t");
  Serial.print(rt);

  Serial.println("");

  Serial.print(ld);
  Serial.print("\t");
  Serial.print(rd);

  Serial.println("\n");

  int dtime = 10; int tol = 30; // dtime=diffirence time, tol=toleransi
  int avt = (lt + rt) / 2; // average value top
  int avd = (ld + rd) / 2; // average value down
  int avl = (lt + ld) / 2; // average value left
  int avr = (rt + rd) / 2; // average value right
  int dvert = avt - avd; // check the diffirence of up and down
  int dhoriz = avl - avr;// check the diffirence og left and rigt


  Serial.println("avt\tavd\tavl\tavr\tdvert\tdhoriz\t");
  Serial.println("---\t---\t---\t---\t---\t---\t");

  Serial.print(avt);
  Serial.print("\t");
  Serial.print(avd);
  Serial.print("\t");
  Serial.print(avl);
  Serial.print("\t");
  Serial.print(avr);
  Serial.print("\t");
  Serial.print(dvert);
  Serial.print("\t");
  Serial.print(dhoriz);

  Serial.println("\n");

  int diff = avr - avl;
  // calculate left/right LDR difference as % of half analogue range
  float pct = (100.00 / (1024 / 2)) * diff;

  Serial.println(diff);
  Serial.println(pct);

  int stepperNewPosition = map(pct, -45,45,0,1024);

  Serial.println(xPos);
  Serial.println(stepperNewPosition);

  int steps = xPos - stepperNewPosition;

  xPos = stepperNewPosition;

  Serial.println(steps);
  
  //Stepper1.step(steps);


  /*
  if (-1*tol > dhoriz || dhoriz > tol) // check if the diffirence is in the tolerance else change horizontal angle
  {
    if (avl > avr)
    {
      Serial.println("avl > avr");
      dir = 1;
      servoh = --servoh;
      if (servoh < servohLimitLow)
      {
        servoh = servohLimitLow;
      }
    } else if (avl < avr)
    {
      Serial.println("avl < avr");
      dir = 2;
      servoh = ++servoh;
      if (servoh > servohLimitHigh)
      {
        servoh = servohLimitHigh;
      }
    }
    else if (avl = avr)
    {
      //delay(1000);
    }
    int rotateSteps = stepsPerHalfRev - map(servoh,5,175,0,stepsPerHalfRev);

    Serial.println(dir);
    Serial.println(servoh);
    Serial.println(rotateSteps);

    if (dir == 2) rotateSteps = rotateSteps*-1;
    
    Serial.println(rotateSteps);

    //Stepper1.step(rotateSteps);
  }

  Servo vertical; // vertical servo
  int servov = 45; 
  int servovLimitHigh = 60;
  int servovLimitLow = 1;

  if (-1*tol > dvert || dvert > tol) 
  {
    if (avt > avd)
    {
      servov = ++servov;
      if (servov > servovLimitHigh)
      {
        servov = servovLimitHigh;
      }
    } else if (avt < avd)
    {
      servov= --servov;
      if (servov < servovLimitLow)
      { 
        servov = servovLimitLow;
      }
    }
    vertical.write(servov);
  }
  */

 
  delay(5000);
}


void setup() 
{
  Serial.begin(115200); // Initialize the serial port

  // Using Port Manipulation ( https://www.arduino.cc/en/Reference/PortManipulation ) 
  DDRD = DDRD | B00011100; // set select pins as OUTPUT
  PORTD = B00011100; // set pins 2,3,4 HIGH

  // setup servo (tilt)
  Servo1.attach(servoPin);

  Stepper1.setSpeed(5);

  Wire.begin();
  Wire.setClock(400000L);

#if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0

  oled.setFont(System5x7);
  oled.clear();
  oled.print("Solar Tracker Init...");

}

void readAnalogVoltage()
{

  while(sample_count < NUM_SAMPLES) {
    sum += analogRead(A0);
    sample_count++;
    delay(10);
  }
  // calculate voltage
  // 5.07 = Arduino calibrated reference voltage, 1024 = ADC max 
  voltage = ((float)sum / (float)NUM_SAMPLES * 5.07) / 1024.0;

  // multiply by calibrated voltage divide  * 11.132
  Serial.print(voltage);
  Serial.println(" V\n");
  
  sample_count = 0;
  sum = 0;

}

void loop() 
{

  readAnalogVoltage();

  // Joystick X,Y position
  int xVal = analogRead(xPin);  // read X axis value [0..1023]
  Serial.print("X:");
  Serial.print(xVal, DEC);

  int yVal = analogRead(yPin);  // read Y axis value [0..1023]
  Serial.print("  Y:");
  Serial.print(yVal, DEC);

  Serial.println();

  // X position
  Serial.print("X:");
  Serial.print(xPos, DEC);

  int xAxisDeg = map(xPos, 0, analogueHigh, 0, 180);
  Serial.print("  X :");
  Serial.print(xAxisDeg, DEC);
  Serial.print(" (deg)");

  Serial.println(" ");

  // Y position
  Serial.print("Y:");
  Serial.print(yPos, DEC);

  int yAxisDeg = map(yPos, 0, analogueHigh, 0, 180);
  Serial.print("  Y:");
  Serial.print(yAxisDeg, DEC);
  Serial.print("  (deg)");

  Serial.println(" ");


  int tol = 30;
  int diffX = abs(xVal - xCentre);
  int diffY = abs(yVal - yCentre);
  int xRotate = 0;

  if (diffX > tol)
  {

    if (xVal < xCentre)
    {
      xPos = xPos - xIncrement;
      if (xPos >= xLimitLow)
      {
        xRotate = xIncrement*-1;
      } else {
        xPos = xLimitLow;
      }
    } else if (xVal > xCentre) {
      xPos = xPos + xIncrement;
      if (xPos <= xLimitHigh)
      {
        xRotate = xIncrement;
      } else {
        xPos = xLimitHigh;
      }
    }


    Serial.print("xRotateSteps:");
    Serial.println(xRotate);

    Stepper1.step(xRotate);

  }

  if (diffY > tol)
  {
    if (yVal < yCentre)
    {
      yPos = yPos - yIncrement;
      if (yPos < yLimitLow)
      {
        yPos = yLimitLow;
      }
    } else if (yVal > yCentre) {
      yPos = yPos + yIncrement;
      if (yPos >= yLimitHigh)
      {
        yPos = yLimitHigh;
      }
    }

    Serial.println("Servo Rotate:");
    Serial.println(yAxisDeg);
    
    Servo1.write(yAxisDeg); 

    Serial.println(" ");
  }



  oled.clear();
  oled.println("Solar Tracker:");
  oled.print("Voltage:");
  oled.print(voltage);
  oled.println(" (v)");
  oled.print("Pan:");
  oled.print(xAxisDeg);
  oled.print("  Tilt:");
  oled.print(yAxisDeg);
  oled.println(" (deg)");

  delay(1000);
}
