/**
 * Simple Serial Transmit/Receieve Client 
 * 
 * Demonstrates device to device simple send/receieve serial RS232 comms
 * includes a JSON msg cmd interpretter / processor:
 * eg. Toggle internal LED: {"cmd":"1004","d":"ON"}
 * 
 * To allow uploading of sketches on Serial, rx/tx pins are shifted 
 *  Arduino Uno - Software Serial library wiring on Digital Pins 6/7 (rx/tx)
 *  Arduino Mega - Hardware Serial RX1/TX1
 * 
 * Transmits LF (0x0A) delimited JSON encoded ASCII text messages
 *
 * Requires: Arduino JSON library
 *
 * Note -
 * Arduino Mega Hardware/Software Serial has a default 64byte buffer configuration
 * 
 * Increase by changing:
 * 
 * ./Arduino/ArduinoCore-avr/cores/arduino/HardwareSerial.h
 * /Arduino/ArduinoCore-avr/libraries/SoftwareSerial/src/SoftwareSerial.h
 * 
 * #define SERIAL_TX_BUFFER_SIZE 256 
 * #define SERIAL_RX_BUFFER_SIZE 256 
 *
 */

#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// CMD codes
#define CMD_READ_SENSOR 1001  // request sensor read
#define CMD_SENS_FREQ  1002   // set sensor read interval 
#define CMD_SEND_SSD 1003     // request sd card data
#define CMD_SET_LED 1004      // toggle internal LED


const int tx_buffer_sz = 256;
char tx_buff[tx_buffer_sz];
const long tx_baud = 115200;

const int rx_buffer_sz = 256;
char rx_buffer[rx_buffer_sz];
uint8_t rx_count;
#define MSG_EOT 0x0A // LF \n 
#define MSG_CMD 0x40 // @ cmd start 

// Arduino Uno
//byte rx = 6;
//byte tx = 7;

// Arduino Mega RX1, TX1
byte rx = 19;
byte tx = 18;

SoftwareSerial s(rx,tx);

void setup() {
  Serial.begin(tx_baud);
  Serial.println("Hello Arduino");

  //Serial1.begin(tx_baud);
  //Serial1.println("Hello Arduino Mega");

  pinMode(rx,INPUT);
  pinMode(tx,OUTPUT);

  // setup Software Serial
  s.begin(tx_baud);
  s.println("Hello Arduino Uno");

  Serial.println("Serial TX/RX BUFFER SIZE: ");
  Serial.println(SERIAL_TX_BUFFER_SIZE);
  Serial.println(SERIAL_RX_BUFFER_SIZE);

  delay(1000);
}

void loop() {
  //readSerialInput();
  //writeSerialOuput();
  sendExampleCmd();
}

/**
 * Write JSON formatted msg to Serial
 */
void writeSerialOuput()
{
  int txMsgId = 0;

  while(1)
  {
    uint32_t ts = millis();
    float tc = random(18,93)/100.0;
    float tf = random(66,73)/100.0;
    float h = random(55,85)/100.0;
    float ldr = random(2,67)/100.0;
    
    serialiseJSON(txMsgId, ts, tc, tf, h, ldr);
    txMsgId++;
    delay(1000);
  } 
}

/**
 * Read Char data from Serial input
 */
void readSerialInput()
{
  int rxMsgId=0;

  while(1)
  {
    // Serial RX Handler
    int b = 0; // EOT break
    rx_count = 0;
    yield(); // disable Watchdog 

    while((b == 0) && (s.available() >= 1))
    {
      char c = s.read();
      if (c >= 0x20 && c <= 0xFF || c == 0x00)
      {
        rx_buffer[rx_count++] = c;
      }
  
      if (c == MSG_EOT || (rx_count == rx_buffer_sz))
      {
        b = 1;
      }
    }
 
    if (rx_count >= 1)
    {
      Serial.println("Serial RX Message: ");
      Serial.println(rx_buffer);
      rxMsgId++;
      clearRxBuffer(rx_buffer);
    }
  }

}

/*
 * Simulate a cmd + data msg 
 */
void sendExampleCmd()
{
  rx_count = 1;
  //const char* json = "{\"cmd\":\"1001\",\"d\":\"ON\"}";
  const char* json = "{\"cmd\":\"1002\",\"d\":\"30000\"}";
  
  copyCharArrayToBuffer(rx_buffer, json, rx_buffer_sz);
  
  if (rx_count >= 1)
  {
    processCmd(rx_buffer);
    clearRxBuffer(rx_buffer);
  }  
}

/**
 * JSON msg command processor - 
 * Extract cmd and data values from JSON format msg
 */
void processCmd(char * rx_buffer)
{
  Serial.println("Serial RX Message: ");
  Serial.println(rx_buffer);

  StaticJsonDocument<256> doc;
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
    case CMD_SEND_SSD :
        Serial.println("Cmd: Send SSD data");
        break;
    case CMD_SENS_FREQ :
        Serial.print("Cmd: Set Sample Freq: ");
        Serial.println(data);
        break;
    default :
        Serial.println("Cmd: Invalid CMD");
  }

}

/**
 * Package an example msg as JSON, send over Serial
 */
void serialiseJSON(uint8_t id, uint32_t ts, float tc, float tf, float h, float ldr)
{

  const size_t capacity = JSON_OBJECT_SIZE(8);
  
  DynamicJsonDocument doc(capacity);
  
  JsonObject data = doc.to<JsonObject>();
  data["id"] = id;
  data["ts"] = ts;
  data["tc"] = tc;
  data["tf"] = tf;
  data["h"] = h;
  data["ldr"] = ldr;

  // write JSON doc to Serial monitor
  serializeJson(doc, Serial);
  Serial.println();

  // write to Serial1 (arduino mega) / Software Serial
  //serializeJson(doc, Serial1);
  //Serial1.println();
  //serializeJson(doc, s);
  //s.println();

  // write JSON doc to char buffer
  //char json_string[256];
  //serializeJson(doc, json_string);

}

void clearRxBuffer(char * rx_buffer)
{
  for(int i=0; i<rx_buffer_sz; i++)
  {
    rx_buffer[i] = '\0';
  }
}

void copyCharArrayToBuffer(char * a, char * b, int length)
{
  //int length = 256;
  //char a[length], b[] = "string";
  
  int i = 0;
  while (i < length && b[i] != '\0') { a[i] = b[i]; i++; }
  a[i] = '\0';
}
