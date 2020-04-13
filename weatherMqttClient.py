### Python MQTT client
### Subscribes to an MQTT topic receiving JSON format messages in format:
###    [{"ts":1586815920,"tempC":22.3,"tempF":72.14,"h":41,"LDR":964,"p":102583,"w":0}]
###
### Concept is to perform real time analytics/processing using pandas/numpy/scipy
### plotting results using matplotlib animation
###
import paho.mqtt.client as mqtt
import json

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import pandas as pd
from datetime import datetime as dt
import seaborn as sns


mqtt_server = "192.168.1.127";
mqtt_port = 1883;
mqtt_keepalive = 60
mqtt_channel_out = "esp8266.out";
mqtt_channel_in = "esp8266.in";

json_rx_data = []

def on_connect(client,userdata,flags,rc):
    print("Connected with result code:"+str(rc))
    print ("MQTT server: "+mqtt_server+", port: "+str(mqtt_port));
    print ("MQTT topic: "+mqtt_channel_out);
    client.subscribe(mqtt_channel_out)

def on_message(client, userdata, msg):
    print(msg.payload)
    parsed_json = (json.loads(msg.payload))
    print(json.dumps(parsed_json, indent=4, sort_keys=True))
    print(parsed_json[0]) # read object
    print(parsed_json[0]["LDR"]) # read object property
    json_rx_data.append(parsed_json[0]) # append recieved json object to list
    print(json.dumps(json_rx_data, indent=4, sort_keys=True))


client = mqtt.Client()
client.connect(mqtt_server,mqtt_port,mqtt_keepalive);

client.on_connect = on_connect
client.on_message = on_message

client.loop_forever()
