/**
  Weather Station

  Example sketch for an Arduino weather station built with an ATmega2560 microcontroller

  Wake on a configurable schedule (every minute) to read sensor sample data

  Hardware includes:
    DS3231 RTC
    DHT11 Temperature & Humidity
    BMP180 Barometric Pressure
    Water Sensor
    SD Card Module
    LDR PhotoResistor 

  ESP8266 provides bidirectional serial IO for network cmd input and data logging
  
  Requires:
    https://github.com/steveio/AlarmSchedule
    https://github.com/steveio/RTCLib

  Notes:
    Hardware Serial buffer ( hardware/arduino/avr/cores/arduino/HardwareSerial.h ) increase to 256k
  
  This example code is in the public domain.
*/

#include <Wire.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <DateTime.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_BMP085.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <SolarPosition.h>
#include <sunMoon.h>

#include <AlarmSchedule.h>
#include <avr/sleep.h>
#include <avr/power.h>


// pinout for peripheral VCC / GND power rail
const int mosfetPin = 9;

const long baud_rate = 115200;


// Serial CMD codes
#define CMD_READ_SENSOR 1001  // request sensor read
#define CMD_SENS_FREQ  1002   // set sensor read interval 
#define CMD_SEND_SSD 1003     // request sd card data
#define CMD_SLEEP 1005        // sleep
#define CMD_WAKEUP 1006       // wakeup
#define CMD_NTP_SYNC 1007     // sync RTC w/ NTP time
#define CMD_LED 1010          // toggle internal LED
#define CMD_SET_LCD 1012      // toggle LCD Display
#define CMD_SET_SDCARD 1013   // toggle SD Card Data logging
#define CMD_SET_WIFI_TX 1014  // toggle Wifi JSON logging


// RTC DS3231
DS3232RTC rtc;
#define RTC_SDA_PIN 20
#define RTC_SCL_PIN 21
#define RTC_INTERRUPT_PIN 2

// interupt flags
volatile int8_t interuptState = 0; // RTC alarm
volatile int8_t serialRxInteruptState = 0; // Serial RX

AlarmScheduleRTC3232 als;



// Serial Messaging IO INT2 / RX1 19
#define RX_INTERRUPT_PIN 19

const int tx_buffer_sz = 256;
char tx_buff[tx_buffer_sz];
const long tx_baud = 115200;

const int rx_buffer_sz = 256;
char rx_buffer[rx_buffer_sz];
uint8_t rx_count;
#define MSG_EOT 0x0A // LF \n 
#define MSG_CMD 0x40 // @ cmd start 
#define MSG_JSON_START 0x7B // { begining JSON cmd
#define MSG_JSON_END 0x7D // } end JSON cmd

// Mega2560 RX1/TX1
byte rxPin = 19;
byte txPin = 18;


DateTime dt;
uint32_t ts;
char dtm[32];

// LCD i2C 
LiquidCrystal_I2C lcd(0x27, 16, 2);

// SD Card - Data Logger
File logFile;
char logFileName[] = "00000000.txt";
const int chipSelect = 53;

// DHT11 Temperature / Humidity
// Wiring (S)ignal | VCC | GND
#define dht_apin 10
DHT dht(dht_apin, DHT11);

// BMP180 Barometric Pressure
Adafruit_BMP085 bmp;
float bmpPressure, bmpTempC, bmpAlt = 0;
float hiThreshold = 102268.9; // Pa / 30.20 inHg
float lowThreshold = 100914.4; // Pa / 30.20 inHg

// LDR - Light Dependant Resistor (S)A0, VCC, -GND, 
int ldr_apin = A1;
int ldr_apin_val = 0;
const int ldr_daynight_threshold = 300; // day/night level
int ldr_adjusted;

// water / moisture sensor
int wSensorPin = A2;
int wSensorVal = 0;

// DHT11 humidity, temp c, temp f
float h, tc, tf; 

// SolarPosition
const uint8_t digits = 3;

// Lat/Long: Bournemouth 50.7192° N, 1.8808° W
#define LOC_latitude    50.7192
#define LOC_longtitude  -1.8808



#define LOC_timezone    60        // UTC difference in minutes  

SolarPosition solarPosition(LOC_latitude, LOC_longtitude);

SolarPosition_t solarPositionData;

sunMoon  sm;

byte mDay;
time_t sRiseT;
time_t sSetT;
char sRise[10];
char sSet[10];

char mp0[] = "New Moon";
char mp1[] = "Waxing Crescent";
char mp2[] = "First Quarter";
char mp3[] = "Waxing Gibbous";
char mp4[] = "Full Moon";
char mp5[] = "Waning Gibbous";
char mp6[] = "Last Quarter";
char mp7[] = "Waning Crescent";

char * moonPhaseLabel[] = {mp0, mp1, mp2, mp3, mp4, mp5, mp6, mp7};

int moonPhaseId[29]=
{
  0, 1, 1, 1, 1, 1, 2, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 6, 6, 6, 6, 6, 6, 7, 8, 8, 8, 8, 8
};


// Daily Hi/Low & Air Pressure Simple moving average

int currDay = dt.day();

// Hi / Low - Indexed by day of week (0 = Sun - 6 = Sat)
float hiT[7] = { 0 }; // T = Temp
float loT[7] = { 0 };
float hiH[7] = { 0 }; // H = Humidity
float loH[7] = { 0 };
float hiP[7] = { 0 }; // P = Air Pressure
float loP[7] = { 0 };


// Air Pressure Avg - 10 * 1 min / 18 * 10 min / 6 * 3 hour Indices
float avgP1Min[10] = { 0 };
float avgP10Min[18] = { 0 };
float avgP3Hour[8] = { 0 };

int avgP1MinCount = 0;
int avgP10MinCount = 0;
int avgP3HourCount = 0;

bool has3hAvg = 0;

int airPressureTrendId = 0;

char ap0[] = "";
char ap1[] = "Steady";
char ap2[] = "Falling";
char ap3[] = "Rising";
char ap4[] = "Falling Rapidly";
char ap5[] = "Rising Rapidly";

char * airPressureTrend[] = {ap0, ap1, ap2, ap3, ap4, ap5};


// struct providing math stats for float sequence
struct arrStat
{
    float min;
    float max;
    float mean;
    int incCount;
    int decCount;
    int incSeqCount;
    int decSeqCount;
    float diff;
    float trend;
    float incMaxDiff;
    float decMaxDiff;
};

struct arrStat result;


/**
 * Simple c array math routine to compute:
 *  - min / max / mean
 *  - range (diff 1st to last element)
 *  - trend (count of ascending / descending values in sequence)
 *  - greatest diff between two elements (increase / decrease)
 *  - count of consecutive (monotonic) sequence (increase / decrease)
 *  - culmulative count of incremental / decrementing sequences
 *
 * @param float array
 * @param int sz number of elements
*  @param struct arraStat (pointer)
 * @return void
 *
 */
void computeArrStats(float arr[], int sz, struct arrStat *result)
{

  // init result struct values
  result->min = 0;
  result->max = 0;
  result->mean = 0;
  result->incSeqCount = 0;
  result->decSeqCount = 0;
  result->incCount = 0;
  result->decCount = 0;
  result->trend = 0;
  result->incMaxDiff = 0;
  result->decMaxDiff = 0;

  result->min = computeMin(arr, sz);
  result->max = computeMax(arr  , sz);
  result->mean = computeAvg(arr, sz);
  result->diff = arr[sz-1] - arr[0]; // difference between last / first data points

  int period = 1;

  // sequential (monotonic) & cumulative increase / decrease counters
  int incSeqCount = 0;
  int decSeqCount = 0;
  int incCumCount = 0;
  int decCumCount = 0;

  for(int i = sz; i > 1; i--)
  {
    float curr = arr[sz - i + period];
    float prev = arr[sz - i];

    float diff = curr - prev;
    if (diff > 0)
    {
      result->trend++;
    } else {
      result->trend--;
    }

    if ( curr > prev) // increase (rise)
    {
      incCumCount++;
      incSeqCount++;
      if (incSeqCount > result->incSeqCount)
      {
        result->incSeqCount = incSeqCount;
      }
      decSeqCount = 0;

      diff = curr - prev;
      if (diff > result->incMaxDiff)
      {
        result->incMaxDiff = diff;
      }

    } else if (curr < prev) { // decrease (fall)
      decCumCount++;
      decSeqCount++;
      if (decSeqCount > result->decSeqCount)
      {
        result->decSeqCount = decSeqCount;
      }
      incSeqCount = 0;

      diff = prev - curr;
      if (diff > result->decMaxDiff)
      {
        result->decMaxDiff = diff;
      }
    }
  }

  result->incCount = incCumCount;
  result->decCount = decCumCount;
}


// compute simple average from array of values
float computeAvg(float v[], uint8_t sz)
{
  // compute average
  float sum = 0, avg;
  for(int i = 0; i<sz; i++)
  {
    sum += v[i];
  }
  return avg = sum / sz;
}

// find max from array of values
float computeMax(float v[], uint8_t sz)
{
  float m = 0;
  for(int i = 0; i<sz; i++)
  {
    m = (v[i] > m) ? v[i] : m;
  }
  return m;
}

// find max from array of values
float computeMin(float v[], uint8_t sz)
{
  float m = 0;
  for(int i = 0; i<sz; i++)
  {
    m = ((i == 0) || (v[i] < m)) ? v[i] : m;
  }
  return m;
}



// Module Config
boolean lcd_active = 1;
boolean sd_active = 1; // enable/disable SD Card Data Logging
boolean wifi_tx_active = 1; // transmit JSON over software serial to ESP8266


void setLCD(boolean b)
{
  lcd_active = b;
}

void setSDCard(boolean b)
{
  sd_active = b;
}

void setWifiTx(boolean b)
{
  wifi_tx_active = b;
}

/**
 * Schedule RTC Alarm
 */
void setAlarm()
{
  Serial.println("setAlarm() - every minute at :00 secs");
  als.setAlarm( ALM1_MATCH_SECONDS, 00, 0, 0, 0, 0);
}


/**
 * Calculate Solar Elevation, Azimuth & Distance
 */
void getSolarPosition()
{
  solarPositionData = solarPosition.getSolarPosition();
  printSolarPosition(solarPosition.getSolarPosition(), digits);
}

/** 
 * Print a solar position to serial 
 */
void printSolarPosition(SolarPosition_t pos, int numDigits)
{
  Serial.println(F("Solar Position: "));

  Serial.print(F("el: "));
  Serial.print(pos.elevation, numDigits);
  Serial.print(F(" deg,\t"));

  Serial.print(F("az: "));
  Serial.print(pos.azimuth, numDigits);
  Serial.println(F(" deg"));
}

void getSunMoon()
{
  mDay = sm.moonDay();
  sRiseT = sm.sunRise();
  sSetT  = sm.sunSet();  

  //day(sRiseT)

  sprintf(sRise, "%02d:%02d",hour(sRiseT), minute(sRiseT));
  sprintf(sSet, "%02d:%02d",hour(sSetT), minute(sSetT));

  Serial.print("Moon age is "); Serial.print(mDay); Serial.println(" day(s)");
  Serial.print("Moon Phase: "); Serial.println(moonPhaseLabel[moonPhaseId[mDay-1]]);
  
  Serial.print("Sunrise and sunset: ");
  Serial.print(sRise);
  Serial.print(" / ");
  Serial.print(sSet);
  Serial.println("");
}


/**
 * Read Samples from Sensor Array
 */
void sensorRead()
{
  getWaterSensor();
  getLDR();
  getDHT11();
  getBMP180();
  getSolarPosition();
  getSunMoon();
}


void getWaterSensor()
{
  wSensorVal = analogRead(wSensorPin);
  Serial.print("Water Sensor: ");
  Serial.println(wSensorVal, DEC);
}

void getLDR()
{
  ldr_apin_val = analogRead(ldr_apin);
  Serial.print("Light Dependant Resistor (LDR): ");
  Serial.println(ldr_apin_val, DEC);
}

void getDHT11()
{

  Serial.println("DHT11: Temp, Humidity ");

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

void printStats()
{

  int i;

  Serial.println("Daily Hi / Lo");

  for (i = 0; i <= 6; i++) 
  {
    Serial.print("Day: ");
    Serial.println(i);

    if (NULL != hiT[i])
    {
      Serial.print("Temp: ");
      Serial.print(hiT[i]);
      Serial.print(" / ");
      Serial.println(loT[i]);
    }

    if (NULL != hiH[i])
    {
      Serial.print("Humidity: ");
      Serial.print(hiH[i]);
      Serial.print(" / ");
      Serial.println(loH[i]);
    }

    if (NULL != hiP[i])
    {
      Serial.print("Air Pressure: ");
      Serial.print(hiP[i]);
      Serial.print(" / ");
      Serial.println(loP[i]);
    }

  }

  Serial.println("");

  Serial.println("Air Pressure Average:");

  Serial.println("1 minute average:");
  for (i = 0; i < 10; i++)
  {
    if (NULL != avgP1Min[i])
    {
      Serial.print(avgP1Min[i]);
      Serial.print("\t");
    }
  }
  Serial.println("");

  Serial.println("10 minute average:");
  for (i = 0; i < 18; i++)
  {
    if (NULL != avgP10Min[i])
    {
      Serial.print(avgP10Min[i]);
      Serial.print("\t");
    }
  }
  Serial.println("");


  Serial.println("3 hour average:");
  for (i = 0; i < 8; i++)
  {
    if (NULL != avgP3Hour[i])
    {
      Serial.print(avgP3Hour[i]);
      Serial.print("\t");
    }
  }
  Serial.println("");

  Serial.println("Air Pressure - 3h statistics:");

  Serial.print("Trend: ");
  Serial.println(airPressureTrend[airPressureTrendId]);


  Serial.print("Min: "); Serial.println(result.min);
  Serial.print("Max: "); Serial.println(result.max);
  Serial.print("Mean: "); Serial.println(result.mean);
  Serial.print("Diff: "); Serial.println(result.diff);
  Serial.print("Trend: "); Serial.println(result.trend);

  Serial.print("Inc Count: "); Serial.println(result.incCount);
  Serial.print("Dec Count: "); Serial.println(result.decCount);

  Serial.print("Inc Seq Count: "); Serial.println(result.incSeqCount);
  Serial.print("Dec Seq Count: "); Serial.println(result.decSeqCount);

  Serial.print("Inc Max Diff: "); Serial.println(result.incMaxDiff);
  Serial.print("Dec Max Diff: "); Serial.println(result.decMaxDiff);

  Serial.println("");

}

// Maintain Metrics: daily hi/low/average
void updateStats()
{

  if (NULL == hiT[dt.dayOfTheWeek()] || tc > hiT[dt.dayOfTheWeek()])
  {
    hiT[dt.dayOfTheWeek()] = tc;
  }
  if (NULL == loT[dt.dayOfTheWeek()] || tc < loT[dt.dayOfTheWeek()])
  {
    loT[dt.dayOfTheWeek()] = tc;
  }

  if (NULL == hiH[dt.dayOfTheWeek()] || h > hiH[dt.dayOfTheWeek()])
  {
    hiH[dt.dayOfTheWeek()] = h;
  }
  if (NULL == loH[dt.dayOfTheWeek()] || h < loH[dt.dayOfTheWeek()])
  {
    loH[dt.dayOfTheWeek()] = h;
  }

  if (NULL == hiP[dt.dayOfTheWeek()] || bmpPressure > hiP[dt.dayOfTheWeek()])
  {
    hiP[dt.dayOfTheWeek()] = bmpPressure;
  }
  if (NULL == loP[dt.dayOfTheWeek()] || bmpPressure < loP[dt.dayOfTheWeek()])
  {
    loP[dt.dayOfTheWeek()] = bmpPressure;
  }

  updateAirPressureAvg();

  printStats();

}

/** 
 * Air Pressure - Simple Moving Average 
 * 
 * Maintain 1min, 10min & 3 Hour Average Indices
 * 
 */
void updateAirPressureAvg()
{

  if (avgP10MinCount >= 1)
  {
    // if at least one 3h avg has rolled up, consider all 18 10 minute values
    int count = (has3hAvg == 1) ? 18 : avgP10MinCount;
    computeArrStats(avgP10Min, count, &result);
    getAirPressureTrend();
  }
  
  avgP1Min[avgP1MinCount++] = bmpPressure;

  if (avgP1MinCount == 10) // roll-up 1 min avg
  {
    // append avg of 1min array to 10 min array
    float a = computeAvg(avgP1Min, avgP1MinCount);
    avgP10Min[avgP10MinCount++] = a;
    avgP1MinCount = 0;
    
    if(avgP10MinCount == 18) // roll-up 10 min avg (18 * 10 mins = 3hour)
    {
      float a2 = computeAvg(avgP10Min, avgP10MinCount);
      avgP3Hour[avgP3HourCount++] = a2;
      has3hAvg = 1;
      avgP10MinCount = 0;

      if (avgP3Hour == 8) // reset 3 hour avg counter
      {
        avgP3HourCount = 0;
      }
    }

  }
}



/** 
 *  Air pressure 3h tendancy / trend
 *  
 */
void getAirPressureTrend()
{
    if (result.diff < -609.55 || result.decMaxDiff > 609.55 || (avgP10MinCount >= 6 && result.trend < (0 - (avgP10MinCount / 3))))
    {
      airPressureTrendId = 4; // falling rapidly
    } else if (result.diff > 609.55 || result.incMaxDiff > 609.55 || (avgP10MinCount >= 6 && result.trend > (avgP10MinCount - (avgP10MinCount / 3))))
    {
      airPressureTrendId = 5; // rising rapidly
    } else if (result.diff < -10.15 && result.trend < -2)
    {
      airPressureTrendId = 2; // falling
    } else if (result.diff > 10.15 && result.trend > 2)
    {
      airPressureTrendId = 3; // rising 
    } else {
      airPressureTrendId = 1; // steady
    }
}


void getBMP180()
{

    Serial.println("BMP180: Air Pressure ");

    Serial.print("Temperature = ");
    bmpTempC = bmp.readTemperature();
    Serial.print(bmpTempC);
    Serial.println(" *C");

    bmpPressure = bmp.readPressure(); 
    Serial.print("Pressure = ");
    Serial.print(bmpPressure);
    Serial.print(" Pa");

    if (bmpPressure > hiThreshold)
    {
      Serial.print(" (High) ");
    } else if (bmpPressure < lowThreshold)  {
      Serial.print(" (Low) ");
    } else {
      Serial.print(" ");
    }

    Serial.println(airPressureTrend[airPressureTrendId]);


    // Calculate altitude assuming 'standard' barometric
    // pressure of 1013.25 millibar = 101325 Pascal
    Serial.print("Altitude = ");
    Serial.print(bmp.readAltitude());
    Serial.println(" meters");

    Serial.print("Pressure at sealevel (calculated) = ");
    Serial.print(bmp.readSealevelPressure());
    Serial.println(" Pa");

    Serial.print("Real altitude = ");
    //Serial.print(bmp.readAltitude(101501));
    bmpAlt = bmp.readAltitude(bmpPressure);
    Serial.print(bmpAlt);
    Serial.println(" meters");
}



void LCDWrite()
{

  /*
  data["ts"] = ts;
  data["t"] = tc;
  data["h"] = h;
  data["l"] = ldr_apin_val;
  data["p"] = bmpPressure;
  data["t2"] = bmpTempC;
  data["a"] = bmpAlt;
  data["w"] = wSensorVal;
  data["el"] = solarPositionData.elevation;
  data["az"] = solarPositionData.azimuth;
  data["lat"] = LOC_latitude;
  data["lon"] = LOC_longtitude;
  data["sr"] = sRise;
  data["ss"] = sSet;
  data["mn"] = mDay;
  */

    long d = 3000;

    lcd.clear();
    lcd.setCursor(0, 0);

    char buff[12];
    sprintf(buff, "%02d/%02d/%02d" , dt.day(),dt.month(),dt.year());
    lcd.print(buff);

    lcd.setCursor(0, 1);
    sprintf(buff, "%02d:%02d:%02d" , dt.hour(),dt.minute(),dt.second());
    lcd.print(buff);


    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);

    lcd.print("Temp / Humidity");

    lcd.setCursor(0, 1);
    float temp = (tc + bmpTempC) / 2;
    lcd.print(temp,2);
    lcd.print((char)223); lcd.print("C  ");
    lcd.print(h);
    lcd.print("%");

    delay(d);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Air Pressure");
    lcd.setCursor(0, 1);
    lcd.print(bmpPressure / 100);
    lcd.print(" Pa");

    if (bmpPressure > hiThreshold)
    {
      lcd.print(" (Hi)");
    } else if (bmpPressure < lowThreshold)  {
      lcd.print(" (Lo)");
    }

    delay(d);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("3h: ");
    lcd.print(result.mean / 100);
    lcd.print(" Pa");

    lcd.setCursor(0, 1);

    Serial.println(airPressureTrend[airPressureTrendId]);

    delay(d);
    lcd.clear();

    /*
    lcd.setCursor(0, 0);
    lcd.print("Light   Water");
    lcd.setCursor(0, 1);
    int l = ((float) (1024 - ldr_apin_val)  / 1024) * 100;
    lcd.print(l,DEC);
    lcd.print("%  ");

    lcd.setCursor(8, 1);
    int w = (((float) (1024 - wSensorVal)  / 1024) * 100) - 100;
    lcd.print(wSensorVal);
    lcd.print("% ");

    delay(d);
    lcd.clear();
    */
    lcd.setCursor(0, 0);
    lcd.print("Sun Position");

    lcd.setCursor(0, 1);
    lcd.print(solarPositionData.azimuth,2);
    lcd.print((char)223);
    lcd.print("  ");
    lcd.print(solarPositionData.elevation,2);
    lcd.print((char)223);

    delay(d);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Sun Rise / Set");
    lcd.setCursor(0, 1);
    lcd.print(sRise);
    lcd.print(" ");
    lcd.print(sSet);

    delay(d);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Moon Phase");
    lcd.print(" (");
    lcd.print(mDay);
    lcd.print(")");

    lcd.setCursor(0, 1);
    lcd.print(moonPhaseLabel[moonPhaseId[mDay-1]]);    

    delay(d);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Temp Hi/Lo: ");

    lcd.setCursor(0, 1);
    lcd.print(hiT[dt.dayOfTheWeek()]);
    lcd.print(" / ");
    lcd.print(loT[dt.dayOfTheWeek()]);
    lcd.print(" ");
    lcd.print((char)223); lcd.print("C  ");

    delay(d);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Humidity Hi/Lo: ");

    lcd.setCursor(0, 1);
    lcd.print(hiH[dt.dayOfTheWeek()]);
    lcd.print(" / ");
    lcd.print(loH[dt.dayOfTheWeek()]);
    lcd.print(" ");
    lcd.print("%");

    delay(d);
    lcd.clear();

}

/**
 * Serialise a single data sample formatted as JSON object
 * If SD Card Module is enabled write data to file
 * 
 */
void serialiseJSON()
{

  const size_t capacity = JSON_OBJECT_SIZE(20);
  
  DynamicJsonDocument doc(capacity);

  JsonObject data = doc.createNestedObject();
  data["ts"] = ts;
  data["t"] = tc;
  data["h"] = h;
  data["l"] = ldr_apin_val;
  data["p"] = bmpPressure;
  data["t2"] = bmpTempC;
  data["a"] = bmpAlt;
  data["w"] = wSensorVal;
  data["el"] = solarPositionData.elevation;
  data["az"] = solarPositionData.azimuth;
  data["lat"] = LOC_latitude;
  data["lon"] = LOC_longtitude;
  data["sr"] = sRise;
  data["ss"] = sSet;
  data["mn"] = mDay;

  char json_string[256];
  serializeJson(doc, json_string, 256);

  if (wifi_tx_active) {\
    Serial1.print(json_string);
    Serial1.println("\n");
    Serial1.flush();

    Serial.println(json_string);
  }

  if (sd_active) {
    SDCardWrite(json_string);
  }

  if (lcd_active)
  {
    LCDWrite();
  }

}

/**
 * Get dated log filename in format YYYYMMDD.txt
 */
void getLogFileName()
{
  sprintf(logFileName, "%02d%02d%02d.txt", dt.year(), dt.month(), dt.day());
}

/**
 * Write data to SD Card
 */
void SDCardWrite(char * str)
{

  if (!SD.begin()) {
    Serial.println("SD init failed!");
    return;
  }

  getLogFileName();

  logFile = SD.open(logFileName, FILE_WRITE);

  if (logFile) 
  {
    logFile.println(str);
    logFile.close();
    Serial.println("SD write OK.");

  } else {
    Serial.println("SD write error");
  }

}

/**
 * Read datafile from SD card and tx to Serial 
 */
void SDCardRead(char * filename)
{
  logFile = SD.open(filename);

  Serial.print("SD read ");
  Serial.println(filename);

  Serial1.begin(baud_rate);
  Serial1.println("\n");
  Serial1.flush();

  long lines = 0;
 
  if (logFile) {

    logFile.seek(0);
    char cr;

    while (logFile.available()) {

      rx_count = 0;
      clearRxBuffer(rx_buffer);

      while(true){

        cr = logFile.read();
        rx_buffer[rx_count++] = cr;

        if(cr == '\n'){
          Serial.println(rx_buffer);
          Serial1.println(rx_buffer);
          Serial1.flush();
          delay(500);
          lines++;
          break;
        }
      }

    }

    Serial.print("Read lines: ");
    Serial.print(lines);
    Serial.println("... Ok");

    logFile.close();

  } else {
    Serial.print("SD File open error");
  }
}

/**
 * Read Char data from Serial input
 */
void readSerialInput()
{
  int rxMsgId=0;

  // Serial RX Handler
  int b = 0; // EOT break
  rx_count = 0;
  yield(); // disable Watchdog 
  byte parseJSONCmd = 0;

  while((b == 0) && (Serial1.available() >= 1))
  {
    char c = Serial1.read();
    if (c == MSG_JSON_START)
    {
      parseJSONCmd = true;
    }

    if (parseJSONCmd)
    {
      Serial.print(c,HEX);
      Serial.print(" ");
      rx_buffer[rx_count++] = c;
    }

    if (c == MSG_EOT || (rx_count == rx_buffer_sz))
    {
      b = 1;
    }
  }

  if (rx_count >= 1)
  {
    rxMsgId++;
    Serial.println(" ");
    Serial.println("Serial RX Message: ");
    Serial.println(rx_buffer);

    processCmd(rx_buffer);
    clearRxBuffer(rx_buffer);
  }

}


void clearRxBuffer(char * rx_buffer)
{
  for(int i=0; i<rx_buffer_sz; i++)
  {
    rx_buffer[i] = '\0';
  }
}



/**
 * JSON msg command processor - 
 * Extract cmd and data values from JSON format msg
 * Message length = 256 bytes
 */
void processCmd(char * rx_buffer)
{

  StaticJsonDocument<rx_buffer_sz> doc;
  deserializeJson(doc,rx_buffer);

  Serial.println("JSON: ");
  serializeJson(doc, Serial);

  Serial.println(" ");
  
  //const char* ccmd = doc["cmd"];
  int cmd = atoi(doc["cmd"]);
  const char* data = doc["d"];

  Serial.print("Cmd: ");
  Serial.println(cmd);
  Serial.print("Data: ");
  Serial.println(data);

  switch(cmd)
  {
    case CMD_READ_SENSOR :
        Serial.println("Cmd: Read Sensor");
        sensorRead();
        serialiseJSON();
        break;
    case CMD_SEND_SSD :
        // read SSD data file and tx to serial 4 mqtt relay
        Serial.println("Cmd: Send SSD data");
        // disable interupts, to prevent timer sensor read / data tx
        cli();
        // extract requested filename
        char * filename = (char *)doc["d"];
        // read file from SD card and TX to serial
        SDCardRead(filename);
        // enable interupts to resume scheduled data logging
        sei(); 
        break;
    case CMD_SENS_FREQ :
        Serial.print("Cmd: Set Sample Freq");
        Serial.println(data);
        break;
    case CMD_SLEEP :
        Serial.print("Cmd: Sleep");
        pinMode( RTC_INTERRUPT_PIN, INPUT_PULLUP);
        pinMode( RX_INTERRUPT_PIN, INPUT_PULLUP);
        setAlarm();
        sleep();
        break;
    case CMD_WAKEUP :
        Serial.print("Cmd: WakeUp");
        wakeUpSerialRx();
        break;
    case CMD_NTP_SYNC :
        Serial.print("Cmd: NTP Sync");
        unsigned long epochTime = (unsigned long)doc["d"];
        rtc.set(epochTime);
        Serial.println("RTC unixtime()");
        DateTime rtc_t = rtc.get();
        Serial.println(rtc_t.unixtime());
        break;
    case CMD_LED :
        Serial.print("Cmd: Toggle LED");
        digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) ^ 1);
        break;
    case CMD_SET_LCD :
        Serial.print("Cmd: SET LCD");
        setLCD((boolean)data);
        break;
    case CMD_SET_SDCARD :
        Serial.print("Cmd: SET SD Card Logging");
        setSDCard((boolean)data);
        break;
    case CMD_SET_WIFI_TX :
        Serial.print("Cmd: SET WIFI TX");
        setWifiTx((boolean)data);
        break;

    default :
        Serial.println("Cmd: Invalid CMD");
  }

}


/**
 * Set RTC clock based on system time
 */
void setRTC()
{

  DateTime rtc_t = rtc.get();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  if (rtc_t.unixtime() < arduino_t.unixtime()) 
  {
    Serial.println("RTC set()");
    rtc.set(arduino_t.unixtime());    

    Serial.println("RTC unixtime()");
    Serial.println(rtc_t.unixtime());

  }
}

/**
 * Set Arduino Internal Clock (sysTime) to compile time 
 */
void setInternalClock()
{
  DateTime compile_t = DateTime(__DATE__, __TIME__);
  setTime(compile_t.unixtime());
  Serial.println(compile_t.unixtime());
}

/*
 * Maintain Arduino system time based on RTC
 */
void setSyncProviderRTC()
{
    setSyncProvider( rtc.get ); // identify the external time provider
    if (timeStatus() != timeSet) 
      Serial.println("Unable to sync with the RTC");
    else
      Serial.println("RTC has set the system time");

    time_t sync_interval = 60*60*24; // once a day
    setSyncInterval(sync_interval); // set the number of seconds between re-sync

}

/**
 * Return ts of last internal clock sync
 */
timeStatus_t getLastClockSync()
{
    timeStatus_t last_sync = timeStatus(); // last clock sync
    Serial.println("Last clock sync: ");
    Serial.println(last_sync);
    return last_sync;
}

/*
 * Return current time_t as input for setSyncProvider( syncProviderRTC );
 */
time_t syncProviderRTC()
{
  DateTime rtc_ts = rtc.get();
  return rtc_ts.unixtime();  
}

void sleep()
{
  Serial.println("sleep()");
  digitalWrite(13, LOW); // turn off internal LED
  digitalWrite(mosfetPin, LOW); // turn off peripheral power rail
  delay(1000);

  power_adc_disable();
  power_spi_disable();
  power_usart0_disable();
  power_usart2_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_timer3_disable();
  power_timer4_disable();
  power_timer5_disable();
  power_twi_disable();

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  pinMode(RTC_INTERRUPT_PIN, INPUT_PULLUP);
  digitalWrite(RTC_INTERRUPT_PIN, HIGH);
  attachInterrupt(0, wakeUpRTCAlarm, LOW);

  pinMode(RX_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RX_INTERRUPT_PIN), wakeUpSerialRx, CHANGE);

  delay(500);

  sleep_mode();
}


void wakeUpRTCAlarm()
{
  wakeUp();
  detachInterrupt(0);
  interuptState = 1;
}

void wakeUpSerialRx()
{
  wakeUp();
  detachInterrupt(1);
  serialRxInteruptState = 1;
}

void wakeUp()
{
  sleep_disable();

  power_adc_enable();
  power_spi_enable();
  power_usart0_enable();
  power_usart2_enable();
  power_timer1_enable();
  power_timer2_enable();
  power_timer3_enable();
  power_timer4_enable();
  power_timer5_enable();
  power_twi_enable();

  digitalWrite(mosfetPin, HIGH);
  digitalWrite(13, HIGH); // turn on internal LED

  Serial1.println(" \n");

  delay(2000);
}

void setup() {

  Serial.begin(baud_rate);
  Serial1.begin(baud_rate);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(rxPin,INPUT);
  pinMode(txPin,OUTPUT);

  pinMode(mosfetPin, OUTPUT);
  digitalWrite(mosfetPin, HIGH);

  delay(3000);

  Serial.println("Weather Station: Begin\n");
  Serial1.println("Arduino Mega2560 Weather Station Begin!");

  lcd.init();
  lcd.backlight();


  pinMode(ldr_apin,INPUT);
  pinMode(ldr_apin,INPUT);

  dht.begin();

  bmp.begin();

  Serial.println("RTCDS3231 Begin!");

  pinMode(RTC_SDA_PIN, INPUT);
  pinMode(RTC_SCL_PIN, INPUT);
  rtc.begin();

  DateTime rtc_t = rtc.get();

  Serial.println(__DATE__);
  Serial.println(__TIME__);

  DateTime arduino_t = DateTime(__DATE__, __TIME__);
  setSyncProviderRTC();

  sm.init(LOC_timezone, LOC_latitude, LOC_longtitude);

  SolarPosition::setTimeProvider(rtc.get);

  dt = rtc.get();
  ts = dt.unixtime(); // unix ts for logging
  sensorRead();
  updateStats();
  serialiseJSON();

  pinMode(RTC_INTERRUPT_PIN, INPUT_PULLUP);
  digitalWrite(RTC_INTERRUPT_PIN, HIGH);
  attachInterrupt(0, wakeUpRTCAlarm, LOW);

  setAlarm();
 
}


void loop() {

  dt = rtc.get();
  ts = dt.unixtime(); // unix ts for logging

  sprintf(dtm, "%02d/%02d/%02d %02d:%02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute(),dt.second());

  if (serialRxInteruptState == 1)
  {
    Serial.println("ISR Serial RX");
    serialRxInteruptState = 0;
  }
  readSerialInput();

  if (interuptState == 1) // ISR called
  {
    Serial.println("ISR RTC Alarm");
    Serial.println(dtm);
    interuptState = 0;

    sensorRead();
    updateStats();
    serialiseJSON();

    pinMode(RTC_INTERRUPT_PIN, INPUT_PULLUP);
    digitalWrite(RTC_INTERRUPT_PIN, HIGH);
    attachInterrupt(0, wakeUpRTCAlarm, LOW);
    setAlarm();

  }

}
