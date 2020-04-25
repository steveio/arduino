#!/usr/bin/env python

# WS server example that synchronizes state across clients

import asyncio
import json
import logging
import websockets
import struct
from pprint import pprint


logging.basicConfig()

STATE = {"value": 0}

USERS = set()


def state_event():
    return json.dumps({"type": "state", **STATE})


def users_event():
    return json.dumps({"type": "users", "count": len(USERS)})


async def notify_state():
    if USERS:  # asyncio.wait doesn't accept an empty list
        binary_data = struct.pack("Ii", 12, STATE['value'])
        await asyncio.wait([user.send(binary_data) for user in USERS])
        ###message = state_event()
        ###await asyncio.wait([user.send(message) for user in USERS])


async def notify_users():
    if USERS:  # asyncio.wait doesn't accept an empty list
        message = users_event()
        await asyncio.wait([user.send(message) for user in USERS])


"""
How to Client ID from 'request_headers':
-- browser
('User-Agent', 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36')
('Origin', 'http://127.0.0.1:3001')
('Sec-WebSocket-Key', 'd+NSm4Dl+wLV5Q+naRlPMA==')

-- arduino
('User-Agent', 'arduino-WebSocket-Client')
('Origin', 'file://')
('Sec-WebSocket-Key', 'b9xcFehjFpQyrcbyWsGUQA==')
('Sec-WebSocket-Protocol', 'arduino')
"""
async def register(websocket):
    USERS.add(websocket)
    await notify_users()


async def unregister(websocket):
    USERS.remove(websocket)
    await notify_users()


async def wsApi(websocket, path):
    # register(websocket) sends user_event() to websocket
    await register(websocket)
    try:
        await websocket.send(state_event())
        async for message in websocket:
            print('Sec-WebSocket-Key: '+websocket.request_headers['Sec-WebSocket-Key'])
            print('MessageType: '+str(type(message)))
            print(message);
            if isinstance(message, (bytes, bytearray)):
                ### Binary: byte array
                ### https://docs.python.org/3.5/library/struct.html
                ### <class 'bytes'>
                ### b'\x0c\x00\x00\x00@\x00\x00\x00'
                ### (12, 64)
                tuple_of_data = struct.unpack("Ii", message)
                print(tuple_of_data)
                cmd = tuple_of_data[0]
                value = tuple_of_data[1]
                STATE["value"] = value
                await notify_state()
            elif isinstance(message, (str)):
                try:
                    data = json.loads(message)
                    if data["action"] == "minus":
                        STATE["value"] -= 1
                        await notify_state()
                    elif data["action"] == "plus":
                        STATE["value"] += 1
                        await notify_state()
                    elif data["action"] == "set":
                        STATE["value"] = int(data["value"])
                        await notify_state()
                    else:
                        logging.error("unsupported event: {}", data)
                except Exception as e:
                    print(e);
    finally:
        await unregister(websocket)


start_server = websockets.serve(wsApi, "192.168.1.127", 6789)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
