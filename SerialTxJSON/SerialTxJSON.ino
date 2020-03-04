/**
 * Simple Serial Transmit Client using Arudion Hardware Serial Library to send char data over RS232
 * 
 * Transmits an LF (0x0A) delimited JSON encoded ASCII text message 
 * 
 * Uses Software Serial library and wiring on Digital Pins 6/7 (rx/tx)
 * 
 * Demonstrates capability to forward a variety of sensor reading data type/values
 */

#include <SoftwareSerial.h>
#include <ArduinoJson.h>

const int tx_buffer_sz = 128;
char tx_buff[tx_buffer_sz];
const long tx_baud = 115200;

byte rx = 6;
byte tx = 7;

SoftwareSerial s(rx,tx);


void serialiseJSON(uint8_t id, uint32_t ts, float tc, float tf, float h, int ldr)
{

  const size_t capacity = JSON_OBJECT_SIZE(6);
  
  DynamicJsonDocument doc(capacity);
  
  JsonObject data = doc.to<JsonObject>();
  data["id"] = id;
  data["ts"] = ts;
  data["tc"] = tc;
  data["tf"] = tf;
  data["h"] = h;
  data["ldr"] = ldr;

  serializeJson(doc, s);
  serializeJson(doc, Serial);

  s.println("\n");
  Serial.println();

}

void setup() {
  Serial.begin(tx_baud);
  Serial.println("Hello Arduino UNO");
  
  s.begin(tx_baud);

  pinMode(rx,INPUT);
  pinMode(tx,OUTPUT);

  delay(1000);
}

void loop() {

  int msgId=0;
  while(1)
  {
    uint32_t ts = millis();
    float tc = 19.21;
    float tf = 66.56;
    float h = 69.00;
    int ldr = 1021;
    
    serialiseJSON(msgId, ts, tc, tf, h, ldr);
    msgId++;
    delay(1000);
  }

}
