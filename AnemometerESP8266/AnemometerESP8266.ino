/**
 * Anemometer - DIY Wind Speed Sensor
 * 
 * OPL550 IR Phototransistor sensor,  OPL240 IR led & optical light chopper interrupter
 * 
 * ESP8266 Version
 * 
 * Phase = Time to complete a single rotation
 * Frequency = Number of rotations in a timed interval
 * RPM = Revolutions per minute
 * m/s = Metres per second
 * Linear Velocity (m/s) = v = 2π / 60 * r * N
 *    where: r = radius (metres), N = number of revolutions per minute
 * Kmph = (3 x π * radius * RPM / 25)
 * 
 * Variables to record: velocity (m/s) / max velocity (m/s), minute / hour / 3 hour average & max
 * Sample Duration / Interval: 3 seconds / 20 samples per minute 
 * Every Minute: Compute Average / Max, store 60 minute totals
 * Every Hour: Compute Hourly Average / Max, store 3 hour totals
 *
 * Data structures:
 * Velocity v (metres / second):
 * uint16_t v[20];        // current velocity @ 3sec intervals, 20 per minute
 * uint16_t vAvgMin[60];  // minute avg velocity
 * uint16_t vAvgHour[3];  // hourly avg velocity 
 * 
 * Protype deploy 1hz AVR timer, then DS3232 RTC for timing
 *
 * Timestamp (secs since epoch)
 * uint16_t ts[20];      // 3sec intervals, 20 per minute
 * uint16_t mMin[60];  // velocity 
 * uint16_t mHour[3];
 *
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <stdint.h>            // has to be added to use uint8_t

const int irSensorPin = 14;

unsigned long currentMillis;
unsigned long previousMillis = 0;
const long interval = 3000; 

volatile long sensorTicks = 0;  // Optical IR sensor pulse counter

#define SAMPLE_FREQ_SEC 20      // 20 * 3 second sample interval 
#define SAMPLE_FREQ_MIN 60      // minute average / max 
#define SAMPLE_FREQ_HOUR 24     // hourly average / max 
#define PI_CONSTANT 3.142
#define SENSOR_RADIUS 0.08  // Radius (mm) from axis to anemometer cup centre
#define SENSOR_TICKS_REV 10 // Optical Sensor Pulses in single revolution

float velocity, maxVelocity;
int secIndex, minIndex, hourIndex = 0;

// Velocity - Average / Max (m/s)
float v[SAMPLE_FREQ_SEC];         // current velocity @ 3sec intervals, 20 samples per minute
float vAvgMin[SAMPLE_FREQ_MIN];  // 60 minute avg velocity
float vAvgHour[SAMPLE_FREQ_HOUR];  // 24 hour avg velocity 
float vMaxMin[SAMPLE_FREQ_MIN];  // 60 minute max velocity
float vMaxHour[SAMPLE_FREQ_HOUR];  // 24 hour max velocity 

typedef struct
{
    float  velocity;
    long   sensorTicks;
    float  rpm;
    float  kmph;
} SensorData;


//const char* ssid = "AndroidAPXXXX";
//const char* password = "__PASS_";


const char* mqtt_server = "192.168.1.127";
const char* mqtt_channel_out = "esp8266.out";
const char* mqtt_channel_in = "esp8266.in";

// WIFI 
WiFiClient espClient;
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE  (256)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {


  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

void reconnect() {

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_channel_out, "hello ESP8266");
      // ... and resubscribe
      client.subscribe(mqtt_channel_in);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// Optical Sensor Tick Counter ISR
ICACHE_RAM_ATTR void sensorTickISR()
{
  sensorTicks++;
}

// ISR event trigger for senstimerTicksor pulse
void setupPinChangeInterrupt()
{
  attachInterrupt(digitalPinToInterrupt(irSensorPin), sensorTickISR, CHANGE);
}

// Periodic Wind Speed Calculator
void processSample()
{

  Serial.print("Sensor Ticks: ");
  Serial.println(sensorTicks);

  float revs = sensorTicks / (float) SENSOR_TICKS_REV;
  float rpm = revs * SAMPLE_FREQ_SEC;

  // calculate linear velocity (metres per second) 
  velocity = rpmToLinearVelocity(rpm);

  Serial.print("Revolutions: ");
  Serial.println(revs);

  Serial.print("RPM: ");
  Serial.println(rpm);

  Serial.print("Velocity (m/s): ");
  Serial.println(velocity);

  if (velocity > maxVelocity) // Update 1 minute max velocity
  {
    maxVelocity = velocity;
  }

  // Todo - Conversions - Kmph/Mph/Beaufort Scale
  float kmph = rpmToKmph(rpm);
  Serial.print("Velocity (kmph): ");
  Serial.println(kmph);

  // record sample to 1 minute velocity array
  v[secIndex++] = velocity;

  SensorData data;
  data.velocity = velocity;
  data.sensorTicks = sensorTicks;
  data.rpm = rpm;
  data.kmph = kmph;

  txData(data);

  sensorTicks = 0;
  revs = 0;
  rpm = 0;
}

void txData(SensorData data)
{  
 
    snprintf (msg, MSG_BUFFER_SIZE, "%.2f,%d,%.2f,%.2f", data.velocity, data.sensorTicks, data.rpm, data.kmph);
    Serial.println(msg);
    client.publish(mqtt_channel_out, msg);
}

// Compute Minute, Hour and Daily Stats = Avg / Max
void updateStats()
{
  if (secIndex == SAMPLE_FREQ_SEC)
  {
    float avg = computeAvg(v, SAMPLE_FREQ_SEC);

    vAvgMin[minIndex] = avg;
    vMaxMin[minIndex] = maxVelocity;

    maxVelocity = 0;
    minIndex++;

    if (minIndex == SAMPLE_FREQ_MIN)
    {
      float avg = computeAvg(vAvgMin, SAMPLE_FREQ_MIN);
      float mx = computeMax(vMaxMin, SAMPLE_FREQ_MIN);

      vAvgHour[hourIndex] = avg;
      vMaxHour[hourIndex] = mx;
      hourIndex++;

      printStats();

      if (hourIndex == SAMPLE_FREQ_HOUR)
      {
          printStats();

          //clearTimer(); // stop simulation end of 24h period

          hourIndex = 0; // reset day counter
      }

      minIndex = 0; // reset hour counter
    }

    secIndex = 0;
  }

}

void printStats()
{
  float avg = computeAvg(vAvgHour, SAMPLE_FREQ_HOUR);
  float mx = computeMax(vMaxHour, SAMPLE_FREQ_HOUR);

  Serial.println("24h Wind Speed Stats: ");
  
  Serial.print("1 day Average: ");
  Serial.println(avg);
    
  Serial.print("1 day Max: ");
  Serial.println(mx);

  Serial.println("Previous Hour: ");

  Serial.println("Minute Average: ");

  for(int i = 0; i<SAMPLE_FREQ_MIN; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(vAvgMin[i]);
  }

  Serial.println("Minute Max: ");

  for(int i = 0; i<SAMPLE_FREQ_MIN; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(vMaxMin[i]);

  }

  Serial.println("Hourly Average: ");

  for(int i = 0; i<SAMPLE_FREQ_HOUR; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(vAvgHour[i]);
  }

  Serial.println("Hourly Max: ");

  for(int i = 0; i<SAMPLE_FREQ_HOUR; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(vMaxHour[i]);

  }

}

// compute simple average from array of values
float computeAvg(float v[], uint8_t freq)
{
  // compute average
  float sum = 0, avg;
  for(int i = 0; i<freq; i++)
  {
    sum += v[i];
  }
  return avg = sum / freq;
}

// find max from array of values
float computeMax(float v[], uint8_t freq)
{
  float mx = 0;
  for(int i = 0; i<freq; i++)
  {
    mx = (v[i] > mx) ? v[i] : mx;
  }
  return mx;
}

float rpmToLinearVelocity(float rpm)
{
  return 2 * PI_CONSTANT / 60 * SENSOR_RADIUS * rpm;
}

float rpmToKmph(float rpm)
{
  return (3 * PI_CONSTANT * SENSOR_RADIUS * rpm / 25);
}

void setup() 
{

  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(irSensorPin, INPUT_PULLUP);

  setupPinChangeInterrupt();

}

void loop() 
{

  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  // calculate wind speed at 3 second intervals
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    Serial.print("Time (ms): ");
    Serial.println(currentMillis - previousMillis);
    
    previousMillis = currentMillis;

    processSample();
  }

}
