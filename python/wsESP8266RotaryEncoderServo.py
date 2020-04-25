#!/usr/bin/env python

# WS server example that synchronizes state across clients

import asyncio
import json
import logging
import websockets
import struct


logging.basicConfig()

STATE = {"angle": 0}

USERS = set()


def state_event():
    return json.dumps({"type": "state", **STATE})


def users_event():
    return json.dumps({"type": "users", "count": len(USERS)})


async def notify_state():
    if USERS:  # asyncio.wait doesn't accept an empty list
        message = state_event()
        my_bytes = bytearray()
        my_bytes.append(STATE['value'],"utf-8")
        await asyncio.wait([user.send(message) for user in USERS])
        await asyncio.wait([user.send(my_bytes) for user in USERS]) // send_binary


async def notify_users():
    if USERS:  # asyncio.wait doesn't accept an empty list
        message = users_event()
        await asyncio.wait([user.send(message) for user in USERS])


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
            print(type(message))
            if isinstance(message, (bytes, bytearray)):
                ### Binary: byte array
                ### https://docs.python.org/3.5/library/struct.html
                ### <class 'bytes'>
                ### b'\x0c\x00\x00\x00@\x00\x00\x00'
                ### (12, 64)
                print(message);
                tuple_of_data = struct.unpack("Ii", message)
                print(tuple_of_data)
                cmd = tuple_of_data[0]
                value = tuple_of_data[1]
            elif isinstance(message, (string)):
                try:
                    data = json.loads(message)
                    if data["cmd"] == "set":
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
