/*
 * WebSocketClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Hash.h>

#include <Servo.h>
int servoPin = 4; 
Servo Servo1;


ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

const char* wifi_ssid = "TALKTALK1F2294";
const char* wifi_pass = "R7EJNAK7";
const char* ws_server = "192.168.1.127";
const uint16_t ws_port = 6789;
const char* ws_path = "/";


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;

    case WStype_CONNECTED: {
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      //webSocket.sendTXT("Connected");
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

      rotateServo(payload);

      // send data to server
      // webSocket.sendBIN(payload, length);
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

void rotateServo(uint8_t * payload)
{
  int angle = *payload;
  Serial.print("RotateServo: ");
  Serial.println(angle);
  Servo1.write(angle);
}

void setup() {
  USE_SERIAL.begin(115200);

  Servo1.attach(2); //D4


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
  
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  //webSocket.enableHeartbeat(15000, 3000, 2);

}

void loop() {
  webSocket.loop();
}
