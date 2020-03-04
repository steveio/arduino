/*
 Serial Rx ESP8266 MQTT Client

 Receives from a serial tx sender on GPIO 13/15 RXD2/TXD2 (Digital Pins D7/8)

 Messages expected as ASCII text with line feed "\n" (0x0A) char delimiter

 Relays messages over Wifi to MQTT server (192.168.1.127, topic: esp8266.out)

 Notes -
  Watchdog timer is disabled using yeild() in rx loop
  Serial GPIO 3/1 RXD0/TXD0 is clear for sketch uploading
 
 To run mosquitto server on Linux Ubuntu:
 mosquitto_sub -h localhost -t esp8266.out

*/

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


byte rx = 13; // GPIO 13 / D7
byte tx = 15; // GPIO 15 / D8

SoftwareSerial s(rx,tx);

const int rx_buffer_sz = 128;
char rx_buff[rx_buffer_sz];
uint8_t rx_count;
#define MSG_EOT 0x0A // LF \n 

const char* ssid = "__WIFI_SSID__";
const char* password = "__WIFI_PASS__";

const char* mqtt_server = "192.168.1.127";
const char* mqtt_channel_out = "esp8266.out";
const char* mqtt_channel_in = "esp8266.in";


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(128)
char msg[MSG_BUFFER_SIZE];
int value = 0;

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
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

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

void setup() {

  Serial.begin(115200);

  delay(100);

  s.begin(115200);
  pinMode(rx,INPUT);
  pinMode(tx,OUTPUT);


  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  rx_count = 0;
  int b = 0; // EOT break

  while(b == 0)
  {
    yield(); // disable Watchdog 
    while(s.available() >= 1)
    {
        char c = s.read();
        if (c >= 0x20 && c <= 0xFF || c == 0x00)
        {
          rx_buff[rx_count++] = c;
        }

        if (c == MSG_EOT)
        {
          b = 1;
        }   
     }
  }

  Serial.println(rx_buff);
  snprintf (msg, MSG_BUFFER_SIZE, "%s", rx_buff);
  client.publish(mqtt_channel_out, msg);
  clearRxBuffer(rx_buff);

}

void clearRxBuffer(char * rx_buffer)
{
  for(int i=0; i<rx_buffer_sz; i++)
  {
    rx_buffer[i] = '\0';
  }
}
