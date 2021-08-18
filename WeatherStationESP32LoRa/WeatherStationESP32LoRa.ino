/*** 
* 
* ESP32 LoRa Weather Station 
* - Wind Vane (16 compass point) 
* - Anemometer (Wind Speed) 
* - DHT11 Temp / Humidity 
* - LDR light level sensor 
* - BMP280 Air Pressure 
* 
* @see https://www.sparkfun.com/products/15901 
* @see https://cdn.sparkfun.com/assets/d/1/e/0/6/DS-15901-Weather_Meter.pdf 
* 
* @requires ArrayStats Library https://github.com/steveio/ArrayStats 
*/

#include <DHT.h>;
#include <Adafruit_BMP280.h> 
//#include <Adafruit_GFX.h> 
//#include <Adafruit_SSD1306.h>
#include <ArrayStats.h>;


// GPIO Pin Mapping 
#define GPIO_DHT 23
#define GPIO_LDR NULL 
#define GPIO_RAIN 32 
#define GPIO_WIND_SPEED 35 
#define GPIO_WIND_VANE 34
#define GPIO_SDA 21 
#define GPIO_SCL 22

// 0.96 OLED LCD 
//#define SCREEN_WIDTH 128 // OLED display width, in pixels 
//#define SCREEN_HEIGHT 32 // OLED display height, in pixels 
//#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)


// Periodic Sample Interval (Anemometer, Rain Bucket Counter)
unsigned long currentMillis; 
unsigned long previousMillis = 0; 
const long sampleInterval = 10000; // sample period (milliseconds)


// DHT22 Temperature / Humidity 
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


// Anemometer
volatile long windSensorTicks = 0; // Reed switch counter
int windSpeedDebounce = 100; 
unsigned long windSpeedLastEvent = 0;

#define PI_CONSTANT 3.142 
#define SENSOR_RADIUS 0.75 // Radius (mm) from axis to anemometer cup centre 
#define SENSOR_TICKS_REV 1 // Sensor "clicks" in single 360 rotation

typedef struct 
{ 
  float velocity; 
  long windSensorTicks; 
  float rpm; 
  float kmph; 
} WindSpeedData;


// Wind Vane 
int vcc = 3; 
float vout = 0; 
int adcres = 4096; 
float r1 = 10000; 
float r2 = 0; 
float buffer = 0; 
int dir = 0;

char d1[] = "N  "; 
char d2[] = "NE "; 
char d3[] = "NNE"; 
char d4[] = "E  "; 
char d5[] = "ENE"; 
char d6[] = "NW "; 
char d7[] = "NNW"; 
char d8[] = "ES "; 
char d9[] = "ESE"; 
char d10[] = "W  "; 
char d11[] = "WNW"; 
char d12[] = "SW "; 
char d13[] = "WSW"; 
char d14[] = "S  "; 
char d15[] = "SSE"; 
char d16[] = "SSW";


char * directionLabel[] = { d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, d16 };

// Rain Bucket 
volatile int rainBucketCounter = 0; 
int rainBucketDebounce = 200; 
unsigned long rainBucketLastEvent = 0;


// Periodic Statistics
#define SAMPLE_FREQ_SEC 6    // number of samples per minute  
#define SAMPLE_FREQ_MIN 60   // ""            ""      hour
#define SAMPLE_FREQ_HOUR 24  // ""            ""      day


void IRAM_ATTR isr_windSpeed() 
{ 
  if (millis() > windSpeedLastEvent + windSpeedDebounce) 
  {   
    windSpeedLastEvent = millis(); 
    windSensorTicks++; 
  }
}

void IRAM_ATTR isr_rainBucket() 
{ 
  if (millis() > rainBucketLastEvent + rainBucketDebounce) 
  { 
    rainBucketCounter++; 
    rainBucketLastEvent = millis(); 
  } 
}

void getWindSpeed() 
{
  //float windSpeed;
  //windSensorStatus = digitalRead(GPIO_WIND_SPEED); 

  Serial.print("Wind Speed: \t");
  Serial.print("Ticks \t");
  Serial.print(windSensorTicks);

  float revs = windSensorTicks / (float) SENSOR_TICKS_REV; 
  float rpm = revs * SAMPLE_FREQ_SEC;

  // calculate linear velocity (metres per second) 
  float velocity = rpmToLinearVelocity(rpm);

  Serial.print("\tRevs: "); 
  Serial.print(revs);
  Serial.print("\tRPM: "); 
  Serial.print(rpm);
  Serial.print("\t Velo (m/s): "); 
  Serial.print(velocity);

  // Todo - Conversions - Kmph/Mph/Beaufort Scale 
  float kmph = rpmToKmph(rpm); 
  Serial.print("\tKmph: "); 
  Serial.println(kmph);

  // @todo record sample to 1 minute velocity array 
  //v[secIndex++] = velocity;
  //windSpeedData.v[secIndex++] = velocity;

  /*
  WindSpeedData data; 
  data.velocity = velocity; 
  data.sensorTicks = windSensorTicks; 
  data.rpm = rpm; 
  data.kmph = kmph;
  */

  windSensorTicks = 0; 
  revs = 0; 
  rpm = 0;
}

float rpmToLinearVelocity(float rpm)
{
  return 2 * PI_CONSTANT / 60 * SENSOR_RADIUS * rpm;
}

float rpmToKmph(float rpm)
{
  return (3 * PI_CONSTANT * SENSOR_RADIUS * rpm / 25);
}

void getWindDirection() 
{
  float windVane;

  windVane = analogRead(GPIO_WIND_VANE); 
  buffer = windVane * vcc; 
  vout = (buffer)/adcres; 
  buffer = (vcc/vout) -1; 
  r2 = r1 * buffer / 1000;

  Serial.print("VANE: "); 
  Serial.print(windVane); 
  Serial.print("\t"); 
  Serial.print(r2); 
  Serial.print("\t");

  /** 
  * Wind Vane Calibration - 3.3v VCC based on 10k resistance voltage divider ) 
  * North is aligned to magnetic north
  *
  Resistance (Ohms):

    N     0.33
    NNE   2
    NE    1.4
    ENE   4-5
    E     3
    ESE   16-17
    ES    13
    SSE   200-230
    S     170-180
    SSW   350-380
    SW    57-58
    WSW   102-104
    W     29
    WNW   37-39
    NW    6
    NNW   7
    
  Sorted List:

    N     0.3
    NE    1.4
    NNE   2
    E     3
    ENE   4 - 5
    NW    6
    NNW   7
    ES    13
    ESE   16 - 17
    W     29
    WNW   37 - 39
    SW    57 - 58
    WSW   102 - 104
    S     170 - 180
    SSE   200 - 230
    SSW   350 - 380

  */
  if (r2 > .3 && r2 < 0.4) { 
    dir = 1; 
  } else if (r2 > 1.39 && r2 < 1.5) { 
    dir = 2; 
  } else if (floor(r2) == 2) { 
    dir = 3; 
  } else if (floor(r2) == 3) { 
    dir = 4; 
  } else if (floor(r2) >= 4 && floor(r2) <= 5) { 
    dir = 5; 
  } else if (floor(r2) == 6) { 
    dir = 6; 
  } else if (floor(r2) == 7) { 
    dir = 7; 
  } else if (floor(r2) == 13) { 
    dir = 8; 
  } else if (floor(r2) >= 16 && floor(r2) <= 17) { 
    dir = 9; 
  } else if (floor(r2) == 29) { 
    dir = 10; 
  } else if (floor(r2) >= 37 && floor(r2) <= 39) { 
    dir = 11; 
  } else if (floor(r2) >= 57 && floor(r2) <= 58) { 
    dir = 12; 
  } else if (floor(r2) >= 102 && floor(r2) <= 104) { 
    dir = 13; 
  } else if (floor(r2) >= 170 && floor(r2) <= 180) { 
    dir = 14; 
  } else if (floor(r2) >= 200 && floor(r2) <= 230) { 
    dir = 15; 
  } else if (floor(r2) >= 350) { 
    dir = 16;
  }

  Serial.print("\t"); 
  Serial.print(dir); 
  Serial.print("\t"); 
  Serial.println(directionLabel[dir-1]);

}

void getRainBucket()
{

  Serial.print("Rain Bucket: ");
  Serial.print("\t");
  Serial.println(rainBucketCounter);

  rainBucketCounter = 0;
}

void getLightLevel() 
{ 
  ldr_apin_val = analogRead(GPIO_LDR); 
  Serial.print("Light Dependant Resistor (LDR): "); 
  Serial.println(ldr_apin_val, DEC); 
}

void getTempHumidity() 
{
  Serial.print("DHT22: ");

  h = dht.readHumidity();
  tc= dht.readTemperature();

  if (isnan(h) || isnan(tc)) { 
    Serial.println("Sensor ERROR"); 
  } else {
    Serial.print("\tTemp (c):"); 
    Serial.print(tc); 
  
    Serial.print("\tHumidity:"); 
    Serial.println(h); 
  }
  //Serial.print("Temperature (fahrenheit):"); 
  //Serial.println(tf);
}


void getAirPressure() 
{
  Serial.print(F("BMP280: "));
  bmpPressure = bmp.readPressure()/100; // pressure in hPa, you can change the unit 
  bmpTempC = bmp.readTemperature(); 
  bmpAlt = bmp.readAltitude(1019.66); // "1019.66" is the pressure(hPa) at sea level in day in your region
  
  Serial.print(F("\tTemp (*C): ")); 
  Serial.print(bmpTempC); 
  Serial.print(F("\tAirP (hPa): ")); 
  Serial.print(bmpPressure); 
  Serial.print(F("\tAlt (m): ")); 
  Serial.println(bmpAlt);
}


void setup() 
{
  Serial.begin(115200);
  Serial.println(F("ESP32 Weather Station (init)..."));
  
  pinMode(GPIO_WIND_VANE, INPUT); 
  pinMode(GPIO_WIND_SPEED, INPUT); 
  pinMode(GPIO_RAIN, INPUT); 
  pinMode(GPIO_LDR, INPUT);
  pinMode(GPIO_DHT, INPUT);

  dht.begin();

  if (!bmp.begin()) { 
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!")); 
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, // Operating Mode. 
  Adafruit_BMP280::SAMPLING_X2, // Temp. oversampling 
  Adafruit_BMP280::SAMPLING_X16, // Pressure oversampling
  Adafruit_BMP280::FILTER_X16, // Filtering.
  Adafruit_BMP280::STANDBY_MS_500); // Standby time.

  rainBucketLastEvent = millis();
  windSpeedLastEvent = millis();
  attachInterrupt(GPIO_WIND_SPEED, isr_windSpeed, FALLING); 
  attachInterrupt(GPIO_RAIN, isr_rainBucket, CHANGE);

  delay(2000);
}

void loop() 
{

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= sampleInterval) {

    // read & display sensors or counters (for interupt measured devices) 
    getTempHumidity(); 
    getAirPressure(); 
    //getLightLevel(); 
    getWindSpeed();
    getWindDirection();
    getRainBucket();

    Serial.println("");

    previousMillis = currentMillis;
  }

}
