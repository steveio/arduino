/**
 * Smart Irrigation
 * 
 * Programable garden / greenhouse / cold frame watering controller
 * 
 *  -- 2x Solenoid valves switched via TiP120 darlington transistor
 *  -- TimedDevice library for scheduling
 *  -- DHT22 for temperature adjusted watering cycle
 * 
 * Hardware:
 * Arduino Nano
 * DS3231 RTC 
 * 2x Solenoid Valve
 * DHT22 Temperature Sensor
 *
 * @requires Library TimedDevice ( https://github.com/steveio/TimedDevice )
 *
 *
 */

#include <Wire.h>
#include "RTClib.h"
#include <DHT.h>;
#include "Relay.h"

// 0.96 OLED SSD1306 Ascii 
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

SSD1306AsciiWire oled;


RTC_DS1307 rtc;
DateTime dt;

unsigned long startTime = 0;
unsigned long sampleInterval = 3000;  // sample frequency in ms
unsigned long sampleTimer = 0;


// Solenoid Valves
#define GPIO_VALVE1 3
#define GPIO_VALVE2 4

Relay valve1(GPIO_VALVE1);
Relay valve2(GPIO_VALVE2);

Timer timer1Cycle, timer2Cycle;
#define SZ_TIME_ELEMENT 3
struct tmElements_t t1_on, t1_off, t2_on, t2_off, t3_on, t3_off;
struct tmElementArray_t timeArray1Cycle, timeArray2Cycle;

// Capacitive Soil Moisture Sensor v1.2
const int SOIL_SENSOR_PIN = A1;
int airVal = 680;
int waterVal = 326;
const int intervals = (airVal - waterVal) / 3;

int soilMoistureVal[2];
int soilMoistureStatusId[2];


// DHT22 Temperature Sensor
#define DHTPIN 5
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);

float h;     // Humidity (%)
float tempC; // Temperature (celcius)
float tempHiThreshold = 18; // defines a "hot" day

// Daily Hi/Low Temperature / Humidity
int currDay = dt.day();

// Hi / Low - Indexed by day of week (0 = Sun - 6 = Sat)
uint8_t hiT[7] = { 0 }; // T = Temp
uint8_t loT[7] = { 0 };
uint8_t hiH[7] = { 0 }; // H = Humidity
uint8_t loH[7] = { 0 };


void setup() {

  Serial.begin(115200);

  startTime = millis();

  Wire.begin();
  Wire.setClock(400000L);

  if (!rtc.begin())
  {
    Serial.println(F("Could not init RTC"));
    while(1);
  }
  dt = rtc.now();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  // sync RTC w/ arduino time @ compile time
  if (dt.unixtime() != arduino_t.unixtime())
  {
    rtc.adjust(arduino_t.unixtime());
  }

  dht.begin();

  pinMode(GPIO_VALVE1, OUTPUT);
  pinMode(GPIO_VALVE2, OUTPUT);
  pinMode(SOIL_SENSOR_PIN, INPUT);


  // Solenoid Valve On/Off times and DayOfWeek
  t1_on.Hour = 8;
  t1_on.Min = 0;
  t1_off.Hour = 8;
  t1_off.Min = 10;

  t2_on.Hour = 19;
  t2_on.Min = 0;
  t2_off.Hour = 19;
  t2_off.Min = 10;

  timeArray1Cycle.n = 1;
  timeArray1Cycle.Wday = 0b01111111; // define which days of week timer is active on

  timeArray1Cycle.onTime[0] = t1_on;
  timeArray1Cycle.offTime[0] = t1_off;

  timer1Cycle.init(TIMER_MINUTE, &timeArray1Cycle);


  timeArray2Cycle.n = 2;
  timeArray2Cycle.Wday = 0b01111111; // define which days of week timer is active on

  timeArray2Cycle.onTime[0] = t1_on;
  timeArray2Cycle.offTime[0] = t1_off;
  timeArray2Cycle.onTime[1] = t2_on;
  timeArray2Cycle.offTime[1] = t2_off;

  timer2Cycle.init(TIMER_MINUTE, &timeArray2Cycle);

  // default to 1 morning watering cycle
  valve1.initTimer(timer1Cycle);

  // OLED LCD Display
  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  oled.setFont(Adafruit5x7);
  oled.clear();

  char dtm[32];
  sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
  oled.println(dtm);

  sampleTimer = millis();
}

void loop() {

  if (millis() >= sampleTimer + sampleInterval)
  {
    char dtm[32];
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
    Serial.println("");
    Serial.println(dtm);
    
    dt = rtc.now();
    
    if (valve1.timer.isScheduled(dt.minute(), dt.hour(), dt.dayOfTheWeek()))
    {
      Serial.println(F("Valve:\ton"));
      valve1.on();
    } else {
      Serial.println(F("Valve:\toff"));
      valve1.off();
    }

    updateStats();
    //printStats();
    updateOLED();

    setWaterCycleFreq();

    sampleTimer = millis();

  }
}

// Track Soild Moisture, Current & daily hi/low temperature / humdity
void updateStats()
{
  h = dht.readHumidity();
  tempC = dht.readTemperature();
  readSoilMoisture(SOIL_SENSOR_PIN, &soilMoistureVal[0], &soilMoistureStatusId[0]);

  if (NULL == hiT[dt.dayOfTheWeek()] || tempC > hiT[dt.dayOfTheWeek()])
  {
    hiT[dt.dayOfTheWeek()] = (int) tempC;
  }

  if (NULL == loT[dt.dayOfTheWeek()] || tempC < loT[dt.dayOfTheWeek()])
  {
    loT[dt.dayOfTheWeek()] = (int) tempC;
  }

  if (NULL == hiH[dt.dayOfTheWeek()] || h > hiH[dt.dayOfTheWeek()])
  {
    hiH[dt.dayOfTheWeek()] = (int) h;
  }
  if (NULL == loH[dt.dayOfTheWeek()] || h < loH[dt.dayOfTheWeek()])
  {
    loH[dt.dayOfTheWeek()] = (int) h;
  }

  if (currDay != dt.dayOfTheWeek())
  {
    currDay = dt.dayOfTheWeek();
    hiT[dt.dayOfTheWeek()] = 0;
    loT[dt.dayOfTheWeek()] = 0;
    hiH[dt.dayOfTheWeek()] = 0;
    loH[dt.dayOfTheWeek()] = 0;
  }
}


// On "hot" days add 2nd evening watering cycle
void setWaterCycleFreq()
{
  if (hiT[dt.dayOfTheWeek()] > tempHiThreshold)
  {
    valve1.initTimer(timer2Cycle);
  } else {
    valve1.initTimer(timer1Cycle);
  }
}

void printStats()
{  
    Serial.print(F("Soil:\t"));
    Serial.print(soilMoistureVal[0]);
    switch(soilMoistureStatusId[0])
    {
      case 2:
        Serial.print(F(" Dry\t"));  
        break;
      case 1:
        Serial.print(F(" Wet\t"));  
        break;
      case 0:
        Serial.print(F(" V Wet\t"));  
        break;
    }
    Serial.println();

    Serial.print(F("TempC:\t"));
    Serial.println(tempC);

    Serial.print(F("H:\t"));
    Serial.println(h);
}

void updateOLED()
{  
    oled.clear();
    oled.set1X();

    char dtm[32];
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
    oled.println(dtm);

    oled.print(F("Valve1:\t"));
    if (valve1.isActive())
    {
      oled.print(F("On"));  
    } else {
      oled.print(F("Off"));
    }

    /*
    oled.print(F(" #2:\t"));
    if (valve2.isActive())
    {
      oled.print(F("on"));  
    } else {
      oled.print(F("off"));
    }
    */

    oled.println("");

    struct tmElementArray_t * _timeArray = valve1.timer.getTimeArray();

    for(int i=0; i<_timeArray->n; i++)
    {
      oled.print(F("On/Off:\t"));
      oled.print(_timeArray->onTime[i].Hour);
      oled.print(F(":"));
      char m[2];
      sprintf(m, "%02u" , _timeArray->onTime[i].Min);
      oled.print(m);
      oled.print(F(" -> "));
      oled.print(_timeArray->offTime[i].Hour);
      oled.print(F(":"));
      sprintf(m, "%02u" , _timeArray->offTime[i].Min);
      oled.println(m);
    }

    oled.print(F("Soil:\t"));
    oled.print((int) soilMoistureVal[0]);

    switch(soilMoistureStatusId[0])
    {
      case 2:
        oled.print(F(" DRY"));
        break;
      case 1:
        oled.print(F(" WET"));  
        break;
      case 0:
        oled.print(F(" V WET"));  
        break;
    }
    oled.println();

    oled.print(F("T:\t"));
    oled.print((int)tempC);
    oled.print(F(" ("));
    oled.print(hiT[dt.dayOfTheWeek()]);
    oled.print(F(" / "));
    oled.print(loT[dt.dayOfTheWeek()]);
    oled.println(F(")"));
    
    oled.print(F("H:\t"));
    oled.print((int)h);
    oled.print(F(" ("));
    oled.print(hiH[dt.dayOfTheWeek()]);
    oled.print(F(" / "));
    oled.print(loH[dt.dayOfTheWeek()]);
    oled.println(F(")"));
}


// take an average from a sensor returning an int value
int readSensorAvg(int pinId, int numSample, int delayTime)
{
  int v = 0;
  for(int i=0; i<numSample; i++)
  {
    v += analogRead(pinId);
    delay(delayTime);
  }

  v = v / numSample;
  return v;
}

/**
 * Read & categorise soil moisture sensor data 
 * @param int pin id
 * @param int soil moisture value (from sensor reading)
 * @param int soil moisture level: { V_WET 0, WET 1, DRY 2, V_DRY 3 }
 */
void readSoilMoisture(int pinId, int * v, int * id)
{

  // take 3 samples with a 200 ms delay
  *v = readSensorAvg(pinId, 3, 200);

  if(*v < airVal && (*v > (airVal - intervals))) // Dry  > 562
  {
    *id = 2;
  }
  else if(*v < airVal && (*v > (airVal - intervals * 2))) // Wet > 444 - 562
  {
    *id = 1;
  }
  else
  {
    *id = 0; // V Wet 326 - 444
  }
}
