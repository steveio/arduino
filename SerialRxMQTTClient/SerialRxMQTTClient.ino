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
byte interuptPin = 12;

SoftwareSerial s(rx,tx);

const long baud_rate = 115200;
const int rx_buffer_sz = 128;
char rx_buff[rx_buffer_sz];
uint8_t rx_count;
#define MSG_EOT 0x0A // LF \n 

const char* ssid = "TALKTALK1F2294";
const char* password = "R7EJNAK7";

const char* mqtt_server = "192.168.1.127";
const char* mqtt_channel_out = "esp8266.out";
const char* mqtt_channel_in = "esp8266.in";


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(256)
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

  // display & relay message to rs232 serial
  digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level

  // wake up Arduino Mega
  digitalWrite(interuptPin, LOW);
  delay(100);

  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    s.print((char)payload[i]);
  }
  Serial.println();
  s.println("\n");

  digitalWrite(interuptPin, HIGH);
  delay(500);

  digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH

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

  Serial.begin(baud_rate);

  delay(100);

  s.begin(baud_rate);
  pinMode(rx,INPUT);
  pinMode(tx,OUTPUT);

  pinMode(interuptPin,OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);

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

  yield(); // disable Watchdog 

  while((b == 0) && (s.available() >= 1))
  {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

    char c = s.read();
    if (c >= 0x20 && c <= 0xFF || c == 0x00)
    {
      rx_buff[rx_count++] = c;
    }

    if (c == MSG_EOT)
    {
      b = 1;
    }
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off
  }

  if (rx_count >= 1)
  {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(rx_buff);
    snprintf (msg, MSG_BUFFER_SIZE, "%s", rx_buff);
    client.publish(mqtt_channel_out, msg);
    clearRxBuffer(rx_buff);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
}

void clearRxBuffer(char * rx_buffer)
{
  for(int i=0; i<rx_buffer_sz; i++)
  {
    rx_buffer[i] = '\0';
  }
}
