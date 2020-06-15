/**
 * Dual Axis Solar Tracker
 * 
 * ATMega328p Solar tracker with pan/tilt based on stepper & microservo motors,
 * Light Sensor 4x LDR (GL5537), interfaced to CD4051be Analog Multiplexer,
 * joystick position control & OLED SSD1306 LCD Display
 * 
 * 
 * References:
 * http://tok.hakynda.com/article/detail/144/cd4051be-cmos-analog-multiplexersdemultiplexers-with-logic-level-conversion-analog-input
 * https://theiotprojects.com/dual-axis-solar-tracker-arduino-project-using-ldr-servo-motors/
 * 
 */

// OLED LCD
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define READ_PIN            A0
#define OLED_RESET          A4
#define LOGO16_GLCD_HEIGHT  16
#define LOGO16_GLCD_WIDTH   16

// create what ever variables you need
double volts;
double bvolts;
double x, y;

// these are a required variables for the graphing functions
bool Redraw = true;

// create the display object
Adafruit_SSD1306 display(OLED_RESET);



// Joystick
int analogueHigh = 1024;
const int xPin = A1;
const int yPin = A2;

int xPos = 512;
int xCentre = 512;
int xLimitLow = 200;
int xLimitHigh = 1024;
int xIncrement = 10;

int yPos = 256;
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

void DrawBarChartH(Adafruit_SSD1306 &d, double curval, double x , double y , double w, double h , double loval , double hival , double inc , double dig, String label, bool &Redraw)
{
  double stepval, mx, level, i, data;

  d.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  d.setTextSize(1);
  d.setCursor(2, 0);
  d.println(label);

  if (Redraw) {
    Redraw = false;

    // step val basically scales the hival and low val to the width
    stepval =  inc * (double (w) / (double (hival - loval))) - .00001;
    // draw the text
    for (i = 0; i <= w; i += stepval) {
      d.drawFastVLine(i + x , y ,  5, SSD1306_WHITE);
      // draw lables
      d.setTextSize(1);
      d.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
      d.setCursor(i + x , y + 4);
      // addling a small value to eliminate round off errors
      // this val may need to be adjusted
      data =  ( i * (inc / stepval)) + loval + 0.00001;
      d.print(data, dig);
    }
  }
  
  // compute level of bar graph that is scaled to the width and the hi and low vals
  // this is needed to accompdate for +/- range capability
  // draw the bar graph
  // write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
  level = (w * (((curval - loval) / (hival - loval))));
  d.fillRect(x + level, y - h, w - level, h,  SSD1306_BLACK);
  d.drawRect(x, y - h, w,  h, SSD1306_WHITE);
  d.fillRect(x, y - h, level,  h, SSD1306_WHITE);
  // up until now print sends data to a video buffer NOT the screen
  // this call sends the data to the screen
  d.display();

}


void DrawDial(Adafruit_SSD1306 &d, double curval, int cx, int cy, int r, double loval , double hival , double inc, double dig, double sa, String label, bool &Redraw) {

  double ix, iy, ox, oy, tx, ty, lx, rx, ly, ry, i, Offset, stepval, data, angle;
  double degtorad = .0174532778;
  static double px = cx, py = cy, pix = cx, piy = cy, plx = cx, ply = cy, prx = cx, pry = cy;

  if (Redraw) {
    //Redraw = false;
    // draw the dial only one time--this will minimize flicker
    d.setTextColor(SSD1306_WHITE,SSD1306_BLACK);
    d.setTextSize(1);
    d.setCursor(cx - r, 1);
    d.println(label);

    // center the scale about the vertical axis--and use this to offset the needle, and scale text
    Offset = (270 +  sa / 2) * degtorad;
    // find hte scale step value based on the hival low val and the scale sweep angle
    // deducting a small value to eliminate round off errors
    // this val may need to be adjusted
    stepval = ( inc) * (double (sa) / (double (hival - loval))) + .00;
    // draw the scale and numbers
    // note draw this each time to repaint where the needle was
    for (i = 0; i <= sa; i += stepval) {
      angle = ( i  * degtorad);
      angle = Offset - angle ;
      ox =  (r - 2) * cos(angle) + cx;
      oy =  (r - 2) * sin(angle) + cy;
      ix =  (r - 6) * cos(angle) + cx;
      iy =  (r - 6) * sin(angle) + cy;
      tx =  (r + 10) * cos(angle) + cx + 8;
      ty =  (r + 10) * sin(angle) + cy;
      d.drawLine(ox, oy, ix, iy, SSD1306_WHITE);
      //d.setTextSize(1);
      //d.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
      //d.setCursor(tx - 10, ty );
      //data = hival - ( i * (inc / stepval)) ;
      //d.println(data, dig);
    }
    for (i = 0; i <= sa; i ++) {
      angle = ( i  * degtorad);
      angle = Offset - angle ;
      ox =  (r - 2) * cos(angle) + cx;
      oy =  (r - 2) * sin(angle) + cy;
      d.drawPixel(ox, oy, SSD1306_WHITE);
    }
  }
  // compute and draw the needle
  angle = (sa * (1 - (((curval - loval) / (hival - loval)))));
  angle = angle * degtorad;
  angle = Offset - angle  ;
  ix =  (r - 10) * cos(angle) + cx;
  iy =  (r - 10) * sin(angle) + cy;
  // draw a triangle for the needle (compute and store 3 vertiticies)
  lx =  1 * cos(angle - 90 * degtorad) + cx;
  ly =  1 * sin(angle - 90 * degtorad) + cy;
  rx =  1 * cos(angle + 90 * degtorad) + cx;
  ry =  1 * sin(angle + 90 * degtorad) + cy;

  // draw a cute little dial center
  d.fillCircle(cx, cy, r - 6, SSD1306_BLACK);

  // then draw the new needle
  d.fillTriangle (ix, iy, lx, ly, rx, ry, SSD1306_WHITE);

  // draw a cute little dial center
  d.fillCircle(cx, cy, 1, SSD1306_WHITE);

  d.display();

}


void setup() 
{
  Serial.begin(115200); // Initialize the serial port

  // Using Port Manipulation ( https://www.arduino.cc/en/Reference/PortManipulation ) 
  DDRD = DDRD | B00011100; // set select pins as OUTPUT
  PORTD = B00011100; // set pins 2,3,4 HIGH

  // setup servo (tilt)
  Servo1.attach(servoPin);

  int yAxisDeg = map(yPos, 0, analogueHigh, 0, 180);

  Servo1.write(yAxisDeg); 


  Stepper1.setSpeed(5);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  display.clearDisplay();
  display.display();

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
  //Serial.print(voltage);
  //Serial.println(" V\n");
  
  sample_count = 0;
  sum = 0;

}

void loop() 
{

  readAnalogVoltage();

  // Joystick X,Y position
  int xVal = analogRead(xPin);  // read X axis value [0..1023]
  //Serial.print("X:");
  //Serial.print(xVal, DEC);

  int yVal = analogRead(yPin);  // read Y axis value [0..1023]
  //Serial.print("  Y:");
  //Serial.print(yVal, DEC);
  //Serial.println();

  // X position
  //Serial.print("X:");
  //Serial.print(xPos, DEC);

  int xAxisDeg = map(xPos, 0, analogueHigh, 0, 180);
  //Serial.print("  X :");
  //Serial.print(xAxisDeg, DEC);
  //Serial.print(" (deg)");
  //Serial.println(" ");

  // Y position
  //Serial.print("Y:");
  //Serial.print(yPos, DEC);

  int yAxisDeg = map(yPos, 0, analogueHigh, 0, 180);
  //Serial.print("  Y:");
  //Serial.print(yAxisDeg, DEC);
  //Serial.print("  (deg)");

  //Serial.println(" ");

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


  char xlabel[8];
  sprintf(xlabel, "pan %d", xAxisDeg);

  // Pan Dial
  DrawDial(display, xAxisDeg, 28, 32, 24, 0, 180 , 45 , 0, 200, xlabel, Redraw);

  char ylabel[8];
  sprintf(ylabel, "tilt %d", yAxisDeg);

  // Tilt Dial
  DrawDial(display, yAxisDeg, 98, 32, 24, 0, 180 , 45 , 0, 200, ylabel, Redraw);

  /*
  char str_temp[6];
  dtostrf(voltage, 4, 2, str_temp);
  char label[16];
  sprintf(label, "Solar %s (v)", str_temp);

  DrawBarChartH(display, voltage, 0, 16, 120, 8, 0, 5, 1, 0, label, Redraw);
  */

  delay(200);
}
