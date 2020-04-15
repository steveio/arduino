### Python MQTT client
### Subscribes to an MQTT topic receiving JSON format messages in format:
###    [{"ts":1586815920,"tempC":22.3,"tempF":72.14,"h":41,"LDR":964,"p":102583,"w":0}]
###
### Writes receieved JSON data to a mongo DB collection
###
import paho.mqtt.client as mqtt
import json
import pymongo

mqtt_server = "192.168.1.127"
mqtt_port = 1883
mqtt_keepalive = 60
mqtt_channel_out = "esp8266.out"
mqtt_channel_in = "esp8266.in"

mongo_server = "mongodb://localhost:27017/"
mongo_db = "weather"
mongo_collection = "sensorData"

def on_connect(client,userdata,flags,rc):
    print("Connected with result code:"+str(rc))
    print ("MQTT server: "+mqtt_server+", port: "+str(mqtt_port));
    print ("MQTT topic: "+mqtt_channel_out);
    client.subscribe(mqtt_channel_out)

def on_message(client, userdata, msg):
    print(msg.payload)
    parsed_json = (json.loads(msg.payload))
    res = sensorData.insert_one(parsed_json[0])
    ##print(json.dumps(parsed_json, indent=4, sort_keys=True))
    ## print(parsed_json[0]) # read object
    ## print(parsed_json[0]["tempC"]) # read object property


mongoClient = pymongo.MongoClient(mongo_server)
mydb = mongoClient[mongo_db]
sensorData = mydb[mongo_collection]


mqttClient = mqtt.Client()
mqttClient.connect(mqtt_server,mqtt_port,mqtt_keepalive);

mqttClient.on_connect = on_connect
mqttClient.on_message = on_message

mqttClient.loop_forever()
