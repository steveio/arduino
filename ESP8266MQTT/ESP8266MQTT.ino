/*

ESP8266 Wifi / MQTT

Implements bidirectional internet JSON messaging API using MQTT as transport protocol
allows device native functions to be called and status / sensor data to be reported 

*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h> 

#include <PubSubClient.h>

#include <ArduinoJson.h>


const char* ssid = "...";
const char* password = "...";

const char* mqtt_server = "192.168.43.185";
const char* mqtt_channel_pub = "esp8266.out";
const char* mqtt_channel_sub = "esp8266.in";


WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (256)
char msg[MSG_BUFFER_SIZE];
int value = 0;

const int rx_buffer_sz = 256;
char rx_buffer[rx_buffer_sz];


#define MSG_JSON_START 0x7B // "{" begining JSON cmd
#define MSG_JSON_END 0x7D // "}" end JSON cmd


#define CMD_READ_SENSOR 1001   // request sensor read
#define CMD_LIGHT_ON  1002     // light on
#define CMD_LIGHT_OFF 1003     // light off
#define CMD_PUMP_ON  1004      // pump on
#define CMD_PUMP_OFF 1005      // pump off


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
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(mqtt_channel_pub, msg);
  }
}


/**
 * JSON msg command processor - 
 * Extract cmd and data values from JSON format msg
 * Msg format:  {"cmd":"","d":""}
 * Message length = 256 bytes
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
  const char* data = doc["d"];

  Serial.print("Cmd: ");
  Serial.println(cmd);
  Serial.print("Data: ");
  Serial.println(data);

  switch(cmd)
  {
    case CMD_READ_SENSOR :
        Serial.println("Cmd: Read Sensor");
        break;
    case CMD_LIGHT_ON :
        Serial.println("Cmd: light on");
        break;
    case CMD_LIGHT_OFF :
        Serial.println("Cmd: light off");
        break;
    case CMD_PUMP_ON :
        Serial.println("Cmd: pump on");
        break;
    case CMD_PUMP_OFF :
        Serial.println("Cmd: pump off");
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
  
}

void lightOn()
{
  
}

void lightOff()
{
  
}

void pumpOn()
{
  
}

void pumpOff()
{
  
}

void sendJSONData()
{

  const size_t capacity = JSON_OBJECT_SIZE(20);
  
  DynamicJsonDocument doc(capacity);

  JsonObject data = doc.createNestedObject();
  
  data["a"] = "";
  data["b"] = "";
  data["c"] = "";

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
