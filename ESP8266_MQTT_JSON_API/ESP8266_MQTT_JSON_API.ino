/*

ESP8266 Wifi / MQTT

Implements bidirectional internet JSON messaging API using MQTT as transport protocol
allows device control functions to be called and status / sensor data to be reported 

Sensors / Peripherals:

  - DHT11 / BMP280:  temp, humidity, air pressure
  - Relay / Switched Devices: light, pump

Examples:

# start mqtt subscriber 
mosquitto_sub -h 192.168.43.185 -t esp8266.out

# request sensor data
mosquitto_pub -h 192.168.43.185 -t esp8266.in -m "{\"cmd\":\"1001\",\"data\":\"\"}" -d -q 1 -i client
# resp:
[{"tc":19.9,"tf":67.82,"h":82,"p":98506.24}]

# lamp on
mosquitto_pub -h 192.168.43.185 -t esp8266.in -m "{\"cmd\":\"1002\",\"data\":\"1\"}" -d -q 1 -i client
# resp:
[{"lamp":1}]

# lamp off
mosquitto_pub -h 192.168.43.185 -t esp8266.in -m "{\"cmd\":\"1002\",\"data\":\"0\"}" -d -q 1 -i client
# resp:
[{"lamp":1}]

# lamp status
mosquitto_pub -h 192.168.43.185 -t esp8266.in -m "{\"cmd\":\"1002\",\"data\":\"\"}" -d -q 1 -i client
# resp:
[{"lamp":1}]




*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h> 

#include <PubSubClient.h>

#include <ArduinoJson.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

// from https://github.com/steveio/TimedDevice
#include "Pump.h"
#include "Relay.h"

const char* ssid = "AndroidAP4026";
const char* password = "c919d73e9374";

const char* mqtt_server = "192.168.43.185";
const char* mqtt_channel_pub = "esp8266.out";
const char* mqtt_channel_sub = "esp8266.in";


WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (256)
char msg[MSG_BUFFER_SIZE];
int value = 0;
const int rx_buffer_sz = MSG_BUFFER_SIZE;
char rx_buffer[rx_buffer_sz];


#define MSG_JSON_START 0x7B // "{" begining JSON cmd
#define MSG_JSON_END 0x7D // "}" end JSON cmd

// MQTT JSON API commands
#define CMD_READ_SENSOR 1001   // request sensor read
#define CMD_LIGHT  1002        // light on/off
#define CMD_PUMP   1004        // pump on/off


// Sensors - DHT11 / BMP280
#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // I2C

#include <DHT.h>
#define dht_apin 2
DHT dht(dht_apin, DHT11);

float h, tc, tf, p; // humidity, temp c, temp f, pressure


// Relay / Switched Devices
#define DEVICE_PUMP1 0
#define DEVICE_LAMP1 2

// GPIO pins
const int r1Pin = 12; // pump relay #1
const int r2Pin = 13; //
const int r3Pin = 14; // lamp relay

// master on/off switches
bool pumpEnabled = true;
bool lampEnabled = true;

// switch off pump if active > pumpTimeout millis
long pumpTimeout = 5000000;

Timer pump1Timer;
// Timer 32 bit bitmask defines hours (from 24h hours) device is on | off
// 0b 00000000 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
long pump1TimerBitmask = 0b00000000000000000000001111100000;
Pump pump1(r1Pin, pumpTimeout);

Timer lamp1Timer;
long lamp1TimerBitmask =0b00000000001111111111111111100000;
Relay lamp1(r3Pin);



void setup_wifi() {

  delay(10);

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
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

// MQTT event callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  clearRxBuffer(rx_buffer);
  int rx_count = 0;

  for (int i = 0; i < length; i++) {
    rx_buffer[rx_count++] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == MSG_JSON_START) {
    processJSONCmd(rx_buffer);
  }

}

void reconnect() {

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_channel_pub, "ESP8266 MQTT hello world");
      client.subscribe(mqtt_channel_sub);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

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


  // Relay GPIO pins
  pinMode(r1Pin, OUTPUT);
  digitalWrite(r1Pin, HIGH);
  pinMode(r2Pin, OUTPUT);
  digitalWrite(r2Pin, HIGH);
  pinMode(r3Pin, OUTPUT);


  // Timer activated devices
  pump1Timer.init(TIMER_HOUR_OF_DAY, &pump1TimerBitmask);
  pump1.initTimer(pump1Timer);

  lamp1Timer.init(TIMER_HOUR_OF_DAY, &lamp1TimerBitmask);
  lamp1.initTimer(lamp1Timer);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  // check pump timeout (in case of network fail or off() msg is lost)
  pump1.checkTimeout();

  /*
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(mqtt_channel_pub, msg);
  }
  */
}


/**
 * JSON msg command processor - 
 * Extract cmd and data values from JSON format msg
 * Msg format:  {"cmd":"","data":""}
 * Message length = MSG_BUFFER_SIZE bytes
 */
void processJSONCmd(char * rx_buffer)
{

  StaticJsonDocument<rx_buffer_sz> doc;
  deserializeJson(doc,rx_buffer);

  Serial.println("JSON: ");
  serializeJson(doc, Serial);

  Serial.println(" ");
  
  //const char* ccmd = doc["cmd"];
  int cmd = atoi(doc["cmd"]);
  const char* data = doc["data"];

  Serial.print("Cmd: ");
  Serial.println(cmd);
  Serial.print("Data: ");
  Serial.println(data);

  switch(cmd)
  {
    case CMD_READ_SENSOR :
        Serial.println("Cmd: Read Sensor");
        readSensor();
        break;
    case CMD_LIGHT :
        Serial.println("Cmd: light on/off");
        if ((int)data[0] == 49)
        {
          lightOn();
        } else if ((int)data[0] == 48) {
          lightOff();
        } else {
          sendDeviceStatus(DEVICE_LAMP1);
        }
        break;
    case CMD_PUMP :
        Serial.println("Cmd: pump on/off");
        if ((int)data[0] == 49)
        {
          pumpOn();
        } else if ((int)data[0] == 48) {
          pumpOff();
        } else {
          sendDeviceStatus(DEVICE_LAMP1);
        }
        break;

    default :
        Serial.println("Cmd: Invalid CMD");
  }

}

/**
 * Device API Methods 
 * 
 */

void readSensor()
{
  Serial.println("readSensor()");


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

  Serial.print("Humidity: ");
  Serial.println(h);
  Serial.print("DHT11 Temp (c): ");
  Serial.println(tc,1);
  Serial.print("DHT11 Temp (f): ");
  Serial.println(tf);


  Serial.print(F("BMP280 Temp (c) : "));
  Serial.println(bmp.readTemperature());

  Serial.print(F("BMP280 Pressure (hPa) : "));
  p = bmp.readPressure();
  Serial.println(p);

  Serial.println("");

  const size_t capacity = JSON_OBJECT_SIZE(20);  
  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject();

  data["tc"] = tc;
  data["tf"] = tf;
  data["h"] = h;
  data["p"] = p;

  sendJSONData(doc);
}

void lightOn()
{
  Serial.println("lightOn()");

  if (lampEnabled)
  {
    if (!lamp1.isActive()) {
      lamp1.on();
    }
  }

  sendDeviceStatus(DEVICE_LAMP1);
}

void lightOff()
{
  Serial.println("lightOff()");

  if (lamp1.isActive()) {
    lamp1.off();
  }

  sendDeviceStatus(DEVICE_LAMP1);
}

void pumpOn()
{
  Serial.println("pumpOn()");

  if (pumpEnabled)
  {
    if (!pump1.isActive()) {
      pump1.on();
    }
  }

  sendDeviceStatus(DEVICE_PUMP1);
}

void pumpOff()
{
  Serial.println("pumpOff()");

  if (pump1.isActive()) {
    pump1.off();
  }

  sendDeviceStatus(DEVICE_PUMP1);
}

// emit a JSON MQTT device status msg 
void sendDeviceStatus(int deviceId)
{
  const size_t capacity = JSON_OBJECT_SIZE(2);  
  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject();

  switch(deviceId)
  {
    case DEVICE_PUMP1 :
      data["pump1"] = (int)pump1.isActive();
      break;
    case DEVICE_LAMP1 :
      data["lamp1"] = (int)lamp1.isActive();
      break;
  }
  sendJSONData(doc);
}

void sendJSONData(DynamicJsonDocument doc)
{

  Serial.println("sendJSONData()");

  /*
  const size_t capacity = JSON_OBJECT_SIZE(20);  
  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject();
  data["a"] = "";
  data["b"] = "";
  data["c"] = "";
  */

  char json_string[MSG_BUFFER_SIZE];
  serializeJson(doc, json_string, MSG_BUFFER_SIZE);

  client.publish(mqtt_channel_pub, json_string);
}

void clearRxBuffer(char * rx_buffer)
{
  for(int i=0; i<rx_buffer_sz; i++)
  {
    rx_buffer[i] = '\0';
  }
}
