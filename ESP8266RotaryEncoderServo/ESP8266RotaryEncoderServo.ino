/**
 * ESP8266 Rotary Encoder Servo Motor 
 * 
 * Demonstrates WIFI bidirectional servo control & position reporting
 * RFC6455 Websocket client / server binary framed connection to a web browser
 * 
 * For Websocket Server (proxy) implementation see: 
 * https://github.com/steveio/arduino/blob/master/python/wsESP8266RotaryEncoderServo.py 
 * 
 * Web Browser Client:
 * 
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

const char* wifi_ssid = "__SSID__";
const char* wifi_pass = "__PASS__";
const char* ws_server = "192.168.1.127";
const uint16_t ws_port = 6789;
const char* ws_path = "/";



// data message
typedef struct data_t
{
  uint8_t cmd;
  int value;
};

// message packaging / envelope
typedef union packet_t {
 struct data_t data;
 uint8_t packet[sizeof(struct data_t)];
};


#define PACKET_SIZE sizeof(struct data_t)

// send / receive msg data structures
union packet_t sendMsg;
union packet_t receiveMsg;

// messaging function prototypes
void readByteArray(uint8_t * byteArray);
void writeByteArray();
void printByteArray();

// buffer 
uint8_t byteArray[PACKET_SIZE];


#define CMD_SERVO_ANGLE 12 // command to report servo position
#define CMD_SERVO_ROTATE 13 // move servo to a specified position

#define outputA 12 // Rotary Encoder #1 CLK
#define outputB 13 // Rotary Encoder #2 DT
//#define outputC 0 // Rotary Encoder button

int counter = 0; // rotary encoder incremental position
int aState; // rotary encoder pin state comparator
int aLastState;  

#include <Servo.h>
int servoPin = 15;
int angle;
int servoStartAngle = 90; 
int limit = 90; // positive/negative range of servo in degrees
Servo Servo1;




void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;

    case WStype_CONNECTED: {
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
    }
      break;
    case WStype_TEXT:
      USE_SERIAL.printf("[WSc] get text: %s\n", payload);

      // send message to server
      // webSocket.sendTXT("message here");
      break;
    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);
      setServoPosition(payload);
      break;

    case WStype_PING:
        // pong will be send automatically
        USE_SERIAL.printf("[WSc] get ping\n");
        break;
    case WStype_PONG:
        // answer to a ping we send
        USE_SERIAL.printf("[WSc] get pong\n");
        break;
    }

}

void rotateServo(int angle)
{
  Serial.print("RotateServo: ");
  Serial.println(angle);
  Servo1.write(angle);
}

// set servo position from Websocket cmd message
void setServoPosition(uint8_t * byteArray)
{
  readByteArray(byteArray);

  printByteArray(receiveMsg);

  int angle = (int) receiveMsg.data.value;
  rotateServo(angle);

  counter = map(angle, 0, 180, -90, 90); // update rotary encoder
}

// send servo position to Websocket server
void sendServoPosition()
{

  sendMsg.data.cmd = CMD_SERVO_ANGLE;
  sendMsg.data.value = angle;

  // write message to buffer
  writeByteArray();

  printByteArray(sendMsg);
  
  webSocket.sendBIN(byteArray, PACKET_SIZE);

}

void readRotaryEncoder()
{  

  aState = digitalRead(outputA);

  /*
  Serial.print("A: ");
  Serial.println(aState);
  Serial.print("B: ");
  Serial.println(digitalRead(outputB));
  */

   if (aState != aLastState){     
     // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
    
    if (digitalRead(outputB) != aState) { 
      if (counter < limit)
      {
        counter ++;
        angle = map(counter, -90, 90, 0, 180);
        rotateServo(angle);
      }
    } else {
      if (counter + limit > 0)
      {
        counter --;
        angle = map(counter, -90, 90, 0, 180);
        rotateServo(angle);
      }
    }

    Serial.print("Position: ");
    Serial.println(counter);

    // transmit servo position
    sendServoPosition();

  } 
  aLastState = aState;

}

void setup() {

    Serial.begin(115200);

    //Serial.setDebugOutput(true);
    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
      USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
      USE_SERIAL.flush();
      delay(1000);
    }

    WiFiMulti.addAP(wifi_ssid, wifi_pass);

    USE_SERIAL.println("Connecting Wifi...");
    if (WiFiMulti.run() == WL_CONNECTED) {
      USE_SERIAL.println("");
      USE_SERIAL.println("WiFi connected");
      USE_SERIAL.println("IP address: ");
      USE_SERIAL.println(WiFi.localIP());
    }
  
    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
      delay(100);
    }
  
    // server address, port and URL
    webSocket.begin(ws_server, ws_port, ws_path);
  
    // event handler
    webSocket.onEvent(webSocketEvent);
  
    // use HTTP Basic Authorization this is optional remove if not needed
    //webSocket.setAuthorization("user", "Password");
  
    // try ever 5000 again if connection has failed
    webSocket.setReconnectInterval(5000);


    Servo1.attach(servoPin);

    rotateServo(0);

    pinMode(outputA,INPUT);
    pinMode(outputB,INPUT);
    //pinMode(outputC,INPUT);
       
    aLastState = digitalRead(outputA);   

}

void loop() {

  readRotaryEncoder();
  webSocket.loop();
}


// display message buffer
void printByteArray(union packet_t msg)
{
  Serial.print(msg.data.cmd);
  Serial.print("\t");
  Serial.println(msg.data.value);
  Serial.println("\t");
}

// read bytes from buffer
void readByteArray(uint8_t * byteArray)
{
  for (int i=0; i < PACKET_SIZE; i++)
  {
    //Serial.println(byteArray[i]);
    receiveMsg.packet[i] = byteArray[i];
  }
}

// write data to buffer
void writeByteArray()
{
  for(int i=0; i<PACKET_SIZE; i++)
  {
    Serial.print(sendMsg.packet[i]);
    Serial.print("\t");
    byteArray[i] = sendMsg.packet[i]; // msg into byte array
  }
  Serial.print("\n");
}
