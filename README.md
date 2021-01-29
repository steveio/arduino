# arduino
Arduino Projects

Repository includes:
  * sketch code for Arduino AVR-C Atmel ATMEGA328p and Espressif ESP8266/32 microcontroller IOT prototypes
  * python data analytics & charts for time series data - Pandas, Numpy, SciPy and Matplotlib 
  * MQTT clients (python/nodeJS) - see weather station below
  * Websocket RFC6455 Client/Server implementations (python/nodeJS/Javacsript)
  * D3.js real time websocket provisioned charts - 

Automated Plant Watering
Standalone configurable system with scheduler & sensors based on Arduino Pro Mini 3v, Capacitive Soil Moisture Sensor, Lamp relay, 5v Pump, OLED LCD display, RTC.
https://github.com/steveio/arduino/tree/master/PlantWater

Weather Station

Arduino Mega2560 sensor based wifi weather station and data logger
https://github.com/steveio/arduino/blob/master/WeatherStation/WeatherStation.ino

MQTT Broker:
Message queue for Weather Station input/output:
mqtt_server = "192.168.1.127";
mqtt_channel_out = "esp8266.out";
mqtt_channel_in = "esp8266.in";

Start cmd: mosquitto_sub -h localhost -t esp8266.out

Python MQTT Client
https://github.com/steveio/arduino/blob/master/weatherMqttClient.py
Writes MQTT weather JSON data to MongoDB datastore
Path: ~/Arduino/projects/python
Start cmd: python weatherMqttClient.py

NodeJS MQTT -> Websocket Relay
Websocket server (for browser clients0, relays MQTT JSON weather data messages
https://github.com/steveio/mqttWebSocket/blob/master/mqttWebsocket.js
Path: ~/Arduino/projects/nodejs/mqttClient
Start cmd: node mqttWebsocket.js

Weather Statistics Web UI 
D3.js Radial Gauges (Websocket Provisioned) for Temperature, Humidity, Air Pressure (including tendancy), Light Level
https://github.com/steveio/mqttWebSocket/blob/master/wsRadialGauge.html
D3.js Barometer Air Pressure displaying current and 3hour moving average) 
https://github.com/steveio/mqttWebSocket/blob/master/wsRadialGaugeV2.html
http://bl.ocks.org/steveio/d549b0610fd489e6a09df8f2aa805ad3
D3.js Liquid Fill Gauges with websocket JSON data provisioning -
https://github.com/steveio/mqttWebSocket/blob/master/wsLiquidFillGauge.html
https://bl.ocks.org/steveio/c7018c8432710ff8df75bf5a0d5cf03f
