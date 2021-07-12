/***
 * 
 * SparkFun Weather Station 
 *  - Wind Vane (16 compass point)
 *  - Anemometer (Wind Speed)
 *
 * @see https://www.sparkfun.com/products/15901
 * @see https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf
 */

 
#define GPIO_WIND_VANE A0
#define GPIO_WIND_SPEED 2


int vcc = 5;
float vout = 0;
float r1 = 10000;
float r2 = 0;
float buffer = 0;
int dir = 0;

char d1[] = "W";
char d2[] = "NW";
char d3[] = "WNW";
char d4[] = "N";
char d5[] = "NNW";
char d6[] = "SW";
char d7[] = "WSW";
char d8[] = "NE";
char d9[] = "NNE";
char d10[] = "S";
char d11[] = "SSW";
char d12[] = "ES";
char d13[] = "SSE";
char d14[] = "E";
char d15[] = "ENE";
char d16[] = "ESE";

char * directionLabel[] = { d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, d16 };


volatile int windSpeedCounter = 0;

void isr_windSpeed()
{
  windSpeedCounter++;
}

void setup() {

  Serial.begin(115200);

  pinMode(GPIO_WIND_VANE, INPUT);
  pinMode(GPIO_WIND_SPEED, INPUT);

  attachInterrupt(digitalPinToInterrupt(GPIO_WIND_SPEED), isr_windSpeed, HIGH);
}

void loop() {

  float windVane, windSpeed;

  windSpeed = digitalRead(GPIO_WIND_SPEED);

  windVane = analogRead(GPIO_WIND_VANE);
  buffer = windVane * vcc;
  vout = (buffer)/1024.0;
  buffer = (vcc/vout) -1;
  r2 = r1 * buffer / 1000;  

/**
 * Wind Vane Calibration - 5v VCC based on 10k resistance voltage divider )
 * 

Resistance (Ohms):

N 3
NNE 15
NE 12
ENE 114 - 116
E 101
ESE 150
ES 46
SSE 72
S 25
SSW 32
SW  6.4
WSW 7
W 1.0
WNW 2.5
NW 1.7
NNW 4.7

Sorted List:

W 1.0
NW 1.7
WNW 2.5
N 3
NNW 4.7
SW  6.4
WSW 7
NE 12
NNE 15
S 25
SSW 32
ES 46
SSE 72
E 101
ENE 114 - 116
ESE 150

 */


  if (r2 > .98 && r2 < 1.2) {
    dir = 1;
  } else if (r2 > 1.4 && r2 < 1.9) {
    dir = 2;
  } else if (r2 > 2.2 && r2 < 2.7) {
    dir = 3;
  } else if (floor(r2) == 3) {
    dir = 4;
  } else if (r2 > 4.5 && r2 < 4.9) {
    dir = 5;
  } else if (r2 > 6.2 && r2 < 6.6) {
    dir = 6;
  } else if (floor(r2) == 7) {
    dir = 7;
  } else if (floor(r2) == 12) {
    dir = 8;
  } else if (floor(r2) == 15) {
    dir = 9;
  } else if (floor(r2) == 25) {
    dir = 10;
  } else if (floor(r2) == 32) {
    dir = 11;
  } else if (floor(r2) == 46) {
    dir = 12;
  } else if (floor(r2) == 72) {
    dir = 13;
  } else if (floor(r2) == 101) {
    dir = 14;
  } else if (floor(r2) > 114 && floor(r2) < 116) {
    dir = 15;
  } else if (floor(r2) == 150) {
    dir = 16;  
  }

  Serial.print("VANE: ");
  Serial.print(directionLabel[dir-1]);
  Serial.print("\t");
  Serial.print(dir);
  Serial.print("\t");
  Serial.print(r2);
  Serial.print("\t");
  Serial.print(windVane);
  Serial.print("\t");
  Serial.print("SPEED: ");
  Serial.print(windSpeed);
  Serial.print("\t");
  Serial.println(windSpeedCounter);

  delay(100);
}
