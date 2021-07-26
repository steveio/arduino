/***
 * 
 * Outdoor Weather Station 
 *  - Wind Vane (16 compass point)
 *  - Anemometer (Wind Speed)
 *  - DHT11 Temp / Humidity
 *  - LDR light level sensor
 *  - BMP280 Air Pressure
 *
 * @see https://www.sparkfun.com/products/15901
 * @see https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf
 */


#include <DHT.h>;

#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



#define GPIO_DHT 13
#define GPIO_LDR NULL
#define GPIO_RAIN 37
#define GPIO_WIND_SPEED 12
#define GPIO_WIND_VANE 38
#define GPIO_SDA 21
#define GPIO_SCL 22


// 0.96 OLED LCD
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)


// DHT11 Temperature / Humidity
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(GPIO_DHT, DHTTYPE);
float h, tc, tf;

// BMP180 Barometric Pressure
Adafruit_BMP280 bmp;
float bmpPressure, bmpTempC, bmpAlt = 0;
float hiThreshold = 102268.9; // Pa / 30.20 inHg
float lowThreshold = 100914.4; // Pa / 30.20 inHg

// LDR - Light Dependant Resistor (S)A0, VCC, -GND, 
int ldr_apin_val = 0;
const int ldr_daynight_threshold = 300; // day/night level
int ldr_adjusted;


// Wind Speed / Dir Vane / Rain Bucket
int vcc = 3;
float vout = 0;
int adcres = 4096;
float r1 = 10000;
float r2 = 0;
float buffer = 0;
int dir = 0;

char d1[] = "W  ";
char d2[] = "NW ";
char d3[] = "WNW";
char d4[] = "N  ";
char d5[] = "NNW";
char d6[] = "SW ";
char d7[] = "WSW";
char d8[] = "NE ";
char d9[] = "NNE";
char d10[] = "S  ";
char d11[] = "SSW";
char d12[] = "ES ";
char d13[] = "SSE";
char d14[] = "E  ";
char d15[] = "ENE";
char d16[] = "ESE";

char * directionLabel[] = { d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, d16 };


volatile int windSpeedCounter = 0;
volatile int rainBucketCounter = 0;
int rainBucketDebounce = 200;
unsigned long rainBucketLastEvent = 0;


void IRAM_ATTR isr_windSpeed()
{
  windSpeedCounter++;
}

void IRAM_ATTR isr_rainBucket()
{
  if (millis() > rainBucketLastEvent + rainBucketDebounce)
  {
    rainBucketCounter++;
    rainBucketLastEvent = millis();
  }
}

void getWindRain()
{

  float windVane, windSpeed, rainBucket; // current values

  windSpeed = digitalRead(GPIO_WIND_SPEED);
  rainBucket = digitalRead(GPIO_RAIN);

  windVane = analogRead(GPIO_WIND_VANE);
  buffer = windVane * vcc;
  vout = (buffer)/adcres;
  buffer = (vcc/vout) -1;
  r2 = r1 * buffer / 1000;  

  Serial.print("VANE: ");
  //Serial.print(directionLabel[dir-1]);

  Serial.print(windVane);
  Serial.print("\t");
  Serial.print(r2);
  Serial.print("\t");

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


  /*
  Serial.print("\t");
  Serial.print(dir);
  Serial.print("\t");
  Serial.print(r2);
  Serial.print("\t");
  Serial.print(windVane);
  */
  Serial.print("\t");
  Serial.print("SPEED: ");
  Serial.print(windSpeed);
  Serial.print("\t");
  Serial.print(windSpeedCounter);

  Serial.print("\t");
  Serial.print("RAIN: ");
  Serial.print(rainBucket);
  Serial.print("\t");
  Serial.println(rainBucketCounter);

}

void getLightLevel()
{
  ldr_apin_val = analogRead(GPIO_LDR);
  Serial.print("Light Dependant Resistor (LDR): ");
  Serial.println(ldr_apin_val, DEC);
}

void getTempHumidity()
{

  Serial.println("DHT22: Temp, Humidity ");

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  tc = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  tf = dht.readTemperature(true);

  if (isnan(h) || isnan(tc)) {
    Serial.println("DHT11 Sensor ERROR");
  }

  Serial.print("Humidity:");
  Serial.println(h);
  Serial.print("Temperature (celsuis):");
  Serial.println(tc);
  Serial.print("Temperature (fahrenheit):");
  Serial.println(tf);

}

void getAirPressure()
{

  Serial.println(F("BMP280 Air Pressure"));

  bmpPressure = bmp.readPressure()/100; // pressure in hPa, you can change the unit
  bmpTempC = bmp.readTemperature();
  bmpAlt = bmp.readAltitude(1019.66); // "1019.66" is the pressure(hPa) at sea level in day in your region


  Serial.print(F("Temperature = "));
  Serial.print(bmpTempC);
  Serial.println(" *C");

  Serial.print(F("Pressure = "));
  Serial.print(bmpPressure);
  Serial.println(" hPa");

  Serial.print(F("Approx altitude = "));
  Serial.print(bmpAlt); 
  Serial.println(" m");
}


void setup() {

  Serial.begin(115200);

  Serial.println(F("Weather Station init"));


  pinMode(GPIO_WIND_VANE, INPUT);
  pinMode(GPIO_WIND_SPEED, INPUT);
  pinMode(GPIO_RAIN, INPUT);
  pinMode(GPIO_LDR, INPUT);

  rainBucketLastEvent = millis();

  attachInterrupt(GPIO_WIND_SPEED, isr_windSpeed, HIGH);
  attachInterrupt(GPIO_RAIN, isr_rainBucket, CHANGE);


  dht.begin();

  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Start the OLED display
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE);
  display.setTextSize(1); 
  display.print("Weather Station Init");
  display.setCursor(32,12);
  display.setTextSize(2);          
  display.println("...."); 
  display.display();
  delay(2000);

}

void loop() 
{

  // read sensors or counters (for interupt measured devices)
  getTempHumidity();
  getAirPressure();
  getLightLevel();
  getWindRain();


/*
    display.clearDisplay();
    float T = bmp.readTemperature();           //Read temperature in C
    float P = bmp.readPressure()/100;         //Read Pressure in Pa and conversion to hPa
    float A = bmp.readAltitude(1019.66);      //Calculating the Altitude, the "1019.66" is the pressure in (hPa) at sea level at day in your region
                                              //If you don't know it just modify it until you get the altitude of your place
    
    display.setCursor(0,0);                   //Oled display, just playing with text size and cursor to get the display you want
    display.setTextColor(WHITE);
    display.setTextSize(2); 
    display.print("Temp");
    
    display.setCursor(0,18);
    display.print(T,1);
    display.setCursor(50,17);
    display.setTextSize(1);
    display.print("C");

    display.setTextSize(1);
    display.setCursor(65,0);
    display.print("Pres");
    display.setCursor(65,10);
    display.print(P);
    display.setCursor(110,10);
    display.print("hPa");

    display.setCursor(65,25);
    display.print("Alt");
    display.setCursor(90,25);
    display.print(A,0);
    display.setCursor(110,25);
    display.print("m");
    
    display.display();
    delay(2000);
*/

  delay(1000);
}
