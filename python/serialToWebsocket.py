#!/usr/bin/python

import serial
import asyncio
import datetime
import random
import websockets

ser = serial.Serial(
port='/dev/ttyACM0',\
baudrate=115200,\
parity=serial.PARITY_NONE,\
stopbits=serial.STOPBITS_ONE,\
bytesize=serial.EIGHTBITS,\
timeout=0)

print("connected to: " + ser.portstr)


async def tx(websocket, path):
    ###now = datetime.datetime.utcnow().isoformat() + "Z"
    line = []
    while True:
        for i in ser.read():
            c = chr(i)
            line.append(c)
            if c == '\n':
                print(''.join(line))
                await websocket.send(''.join(line))
                line = []
                break

start_server = websockets.serve(tx, "127.0.0.1", 5678)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()



ser.close()
