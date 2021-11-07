/** 
 *  Smart Indoor Gardening
 * 
 *  -- Scheduled (relay) activation for LED LAMP(s)
 *  -- Scheduled 5v pump activation for watering
 *  -- Soil moisture / temperature monitoring
 * 
 * Arduino Pro Mini 3.3v 
 * 2x 5v water pump attached via relay
 * RTC DS1307
 * Lamp via mains 240v AC relay
 * 2x Soil Moisture Capacitive v1.2 sensor
 * DHT22 Temperature / Humidity Sensor
 * 
 * Minimal (low memory) standalone mcu optimised
 *
 * @requires Library TimedDevice ( https://github.com/steveio/TimedDevice )
 *
 */

#include <Wire.h>
#include "RTClib.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <DHT.h>;
#include "Pump.h"
#include "Relay.h"


// SSD1306 Ascii 
// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C
#define RST_PIN -1
SSD1306AsciiWire oled;

volatile int dspRefresh = 0; // 1hz counter
int dspIndex = 0;  // display screen index


// DHT22 Temperature / Humidity
#define DHTPIN 23
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

float humidity, tempC;


RTC_DS1307 rtc;
DateTime dt;

#define VERSION_ID 1.0

unsigned long startTime = 0;
unsigned long calibrationTime = 1500; // time for sensor(s) to calibrate
unsigned long sampleInterval = 10000;  // sample frequency in ms
unsigned long sampleTimer = 0;


/*
 * GPIO Device Mappings
 * 
 */

// Pins & mapping to devices
const int r1Pin = 10; // pump relay #1
const int r2Pin = 11; // pump relay #2
const int r3Pin = 12; // lamp relay
const int s1Pin = A1; // soil moisture sensor #1
const int s2Pin = A2; // soil moisture sensor #2


// master on/off switches
bool pumpEnabled = false;
bool lampEnabled = true;


/*
 * Pump timer schedule uses a combination of:
 *  Hour of day bitmask + duration + delay to define activation window
 *  Recurring timer active 1 day every n days 
 */
long pumpDuration = 30000; // 30 sec activation
long pumpDelay = 3600000; // 1 hour delay between activations


// Timer 32 bit bitmask defines hours (24h) device is on | off
// 0b 00000000 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

long pumpTimerHourBitmask = 0b00000000001000000000001000000000; // 8am + 8pm 

Timer pump1Timer;
Pump pump1(r1Pin, pumpDuration, pumpDelay); // (<pin-id> <duration> <delay-between-activations>)

Timer pump2Timer;
Pump pump2(r2Pin, pumpDuration, pumpDelay);


Timer lamp1Timer;
long lamp1TimerBitmask =0b00000000000111111111111111000000; // 6am - 8pm
Relay lamp1(r3Pin);


// Soil Moisture Capacitive v1.2 sensor calibration
int airVal = 680;
int waterVal = 326;
const int intervals = (airVal - waterVal) / 3;

int soilMoistureVal[2];
int soilMoistureStatusId[2];


ISR(TIMER1_COMPA_vect)
{
  dspRefresh++;
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

void readDHT22()
{
    humidity = dht.readHumidity();
    tempC = dht.readTemperature();
}

void displayOLED()
{

    oled.clear();
    oled.set1X();

    char dtm[32], buf[2];
    int nextEvent;
    
    sprintf(dtm, "%02d/%02d/%02d %02d:%02d" , dt.day(),dt.month(),dt.year(),dt.hour(),dt.minute());
    oled.println(dtm);

    oled.set2X();

    if (dspIndex == 0)
    {
      // LAMP 1
      oled.print(F("Lamp1: "));
      if (lamp1.isActive())
      {
        oled.print("1");
      } else {
        oled.print("0");
      }
  
      oled.print("/");
  
      if (lamp1.timer.isScheduled(dt.hour()))
      {
        oled.print("1");
      } else {
        oled.print("0");
      }
  
      oled.print("/");
  
      nextEvent = lamp1.timer.getNextEvent(dt.hour());
      sprintf(buf,"%02d",nextEvent);
      oled.print(buf);
      oled.println(":00");

    } else if (dspIndex == 1)
    {

      // PUMP 1
      oled.print(F("Pump1: "));
      if (pump1.isActive())
      {
        oled.print("1");
      } else {
        oled.print("0");
      }
  
      oled.print("/");
  
      if (pump1.timer.isScheduled(dt.hour()))
      {
        oled.print("1");
      } else {
        oled.print("0");
      }
  
      oled.print("/");
  
      nextEvent = pump1.timer.getNextEvent(dt.hour());
      sprintf(buf,"%02d",nextEvent);
      oled.print(buf);
      oled.println(":00");
  
      oled.println("");

      // PUMP 2
      oled.print(F("Pump 2: "));
      if (pump2.isActive())
      {
        oled.print("1");
      } else {
        oled.print("0");
      }
  
      oled.print("/");
  
      if (pump2.timer.isScheduled(dt.hour()))
      {
        oled.print("1");
      } else {
        oled.print("0");
      }
  
      oled.print("/");
  
      nextEvent = pump2.timer.getNextEvent(dt.hour());
      sprintf(buf,"%02d",nextEvent);
      oled.print(buf);
      oled.println(":00");
  
    } else if (dspIndex == 2)
    {


      oled.print(F("Soil1: "));
      sprintf(buf,"%02d",soilMoistureVal[0]);
      oled.print(buf);
      oled.print("/");
      sprintf(buf,"%02d",soilMoistureStatusId[0]);
      oled.print(buf);
  
      oled.println("");

      oled.print(F("Soil2: "));
      sprintf(buf,"%02d",soilMoistureVal[1]);
      oled.print(buf);
      oled.print("/");
      sprintf(buf,"%02d",soilMoistureStatusId[1]);
      oled.print(buf);
  
      oled.println("");

      oled.println(F("Temp/Hum: "));
      oled.print(tempC);
      oled.print((char)223);
      oled.print(F("C / "));
      oled.print(humidity);
      oled.print(F("%"));
  
      oled.println("");

    }
}

void setup() {

  Serial.begin(115200);

  startTime = millis();

  Wire.begin();
  Wire.setClock(400000L);

  rtc.begin();
  dt = rtc.now();
  DateTime arduino_t = DateTime(__DATE__, __TIME__);

  // sync RTC w/ arduino time @ compile time
  if (dt.unixtime() != arduino_t.unixtime())
  {
    rtc.adjust(arduino_t.unixtime());
  }

  // Sensor / Relay (pump/lamp) Pins
  pinMode(s1Pin, INPUT);
  pinMode(s2Pin, INPUT);

  pinMode(r1Pin, OUTPUT);
  digitalWrite(r1Pin, HIGH);
  pinMode(r2Pin, OUTPUT);
  digitalWrite(r2Pin, HIGH);

  pinMode(r3Pin, OUTPUT);
  pinMode(r3Pin, HIGH);

  pump1Timer.init(TIMER_HOUR_OF_DAY, &pumpTimerHourBitmask);
  pump1.initTimer(pump1Timer);
  
  pump2Timer.init(TIMER_HOUR_OF_DAY, &pumpTimerHourBitmask);
  pump2.initTimer(pump2Timer);
  
  lamp1Timer.init(TIMER_HOUR_OF_DAY, &lamp1TimerBitmask);
  lamp1.initTimer(lamp1Timer);

  dht.begin();

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

  // setup 1hz timer to cycle OLED display
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 62500 - 1;
  TCCR1B = _BV(WGM12)    // CTC mode, TOP = OCR1A
         | _BV(CS12);    // clock at F_CPU/256
  TIMSK1 = _BV(OCIE1A);  // interrupt on output compare A

  sampleTimer = millis();
}

void loop() {


  if (millis() >= sampleTimer + sampleInterval)
  {

    // check lamp status for current hour and (de)activate
    dt = rtc.now();

    if (lampEnabled)
    {
      if (lamp1.timer.isScheduled(dt.hour(), NULL)) {
        lamp1.on();
      } else if (!lamp1.timer.isScheduled(dt.hour(), NULL)) {
        lamp1.off();
      }
    }

    // read soil moisture sensors: soilMoistureStatusId: { V_WET 0, WET 1, DRY 2, V_DRY 3 }
    readSoilMoisture(s1Pin, &soilMoistureVal[0], &soilMoistureStatusId[0]);
    readSoilMoisture(s2Pin, &soilMoistureVal[1], &soilMoistureStatusId[1]);

    if (pumpEnabled && pump1.timer.isScheduled(dt.hour(), dt.unixtime()))
    {
      pump1.activate(dt.hour());
    }

    if (pumpEnabled && pump1.timer.isScheduled(dt.hour(), dt.unixtime()))
    {
      pump2.activate(dt.hour());
    }

    // check for pump deactivation
    pump1.deactivate(dt.hour(), dt.dayOfTheWeek());
    pump2.deactivate(dt.hour(), dt.dayOfTheWeek());

    readDHT22();

    sampleTimer = millis();
  }

  if (dspRefresh == 5)
  {
    dspIndex++;
    displayOLED();

    if (dspIndex == 2)
    {
      dspIndex = 0;
    }

    dspRefresh = 0;
  }
}
