# -*- coding: utf-8 -*-
"""
Notifications
-------------

Example showing how to add notifications to a characteristic and handle the responses.

Updated on 2019-07-03 by hbldh <henrik.blidh@gmail.com>

"""

import argparse
import asyncio
import datetime
import json
import logging
import sys
import time

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

logger = logging.getLogger(__name__)


PHONE_ADDRESS = "B4:56:E3:B8:76:DA"
AMS_UUID =             '89D3502B-0F36-433A-8EF4-C502AD55F8DC' # Apple Media Service
REMOTE_COMMAND_UUID =  '9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2' # Remote Command
ENTITY_UPDATE_UUID =   '2F7CABCE-808D-411F-9A0C-BB92BA96C102' # Entity update
ENTITY_ATTR_UUID =     'C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7' # Entity attribute


class ENTITY_ATTR:
    def __init__(self, data):
        self.EntityID = int(data[0])
        self.AttributeID = int(data[1])
        self.EntityUpdateFlags = int(data[2])
        self.Value = bytes(data[3:]).decode()
        global Entity

global Entity
Entity = {
    'EntityIDPlayer': {
        'PlayerAttributeIDName': None,
        'PlayerAttributeIDPlaybackInfo': None,
        # {
            # 'PlaybackState': None,
            # 'PlaybackRate': None,
            # 'ElapsedTime': None,
        # },
        'PlayerAttributeIDVolume': None,
    },
    'EntityIDQueue': {
        'QueueAttributeIDIndex': None,
        'QueueAttributeIDCount': 0,
        'QueueAttributeIDShuffleMode': 0,
        'QueueAttributeIDRepeatMode': 0
    },
    'EntityIDTrack': {
        'TrackAttributeIDArtist': None,
        'TrackAttributeIDAlbum': None,
        'TrackAttributeIDTitle': None,
        'TrackAttributeIDDuration': None
    },
    'ReciveTime': {
        'ReciveTime': None
    },
    'Connected': {
        'Connected': True
    }
}

def writeOut(repeat=False):
    if not Entity['Connected']['Connected']:
        sys.stdout.write(json.dumps({
            "text": "Not connected",
            "alt": "not-connected",
            "tooltip": "Not connected"
        }) + "\n")
        sys.stdout.flush()
        return True
    try:
        if int(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'][0]) == 0:
            state = "paused"
            icon = "▶ "
        elif int(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'][0]) == 1:
            state = "playing"
            icon = "⏸ "

        volume = str(int(float(Entity['EntityIDPlayer']['PlayerAttributeIDVolume'])*16)) + "/16"

        duration = time.strftime('%M:%S', time.gmtime(float(Entity['EntityIDTrack']['TrackAttributeIDDuration'])))

        # CurrentElapsedTime = ElapsedTime + ((TimeNow – TimePlaybackInfoWasReceived) * PlaybackRate)
        CurrentElapsedTime = float(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'][2]) + ((time.time() - datetime.datetime.strptime(Entity['ReciveTime']['ReciveTime'], '%Y-%m-%d %H:%M:%S.%f').timestamp()) * float(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'][1]))
        elapsed = time.strftime('%M:%S', time.gmtime(float(CurrentElapsedTime+1)))

        out = {
            "text": Entity['EntityIDTrack']['TrackAttributeIDTitle'] + " - " + elapsed + "/" + duration + " - " + volume,
            "class": state,
            "alt": state,
            "tooltip": 
    f"""{Entity['EntityIDTrack']['TrackAttributeIDTitle']} - {elapsed}/{duration}
    {Entity['EntityIDTrack']['TrackAttributeIDAlbum']} - {Entity['EntityIDTrack']['TrackAttributeIDArtist']}""".replace("&", "&amp;")
        }
    except:
        out = {
            "text": "Not connected",
            "alt": "not-connected",
            "tooltip": "Not connected"
        }
    sys.stdout.write(json.dumps(out) + "\n")
    sys.stdout.flush()
    # if repeat:
    #     time.sleep(0.5)
    #     writeOut(repeat)
    return True


def notification_handler(characteristic: BleakGATTCharacteristic, data: bytearray):
    """Simple notification handler which prints the data received."""
    # logger.info("%s: %r", characteristic.description, data)
    # print(data[4])
    value = ENTITY_ATTR(data)
    one = list(Entity.keys())[value.EntityID]
    two = list(Entity[one].keys())[value.AttributeID]
    if two == 'PlayerAttributeIDPlaybackInfo' and Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo']:
        if (Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'][0] == tuple(map(float, str(value.Value).split(',')))[0]) and (Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'][2] != tuple(map(float, str(value.Value).split(',')))[2]):
            return
    Entity[one][two] = value.Value
    # print(str(datetime.datetime.now()))
    Entity['ReciveTime']['ReciveTime'] = str(datetime.datetime.now())
    # print(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'])
    if (one == 'EntityIDPlayer') and (two == 'PlayerAttributeIDPlaybackInfo'):
        Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'] = tuple(map(float, str(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo']).split(',')))
    # print(Entity)
    # print('\n')
    writeOut()


async def main():
    logger.info("starting scan...")

    # if args.address:
    device = await BleakScanner.find_device_by_address(PHONE_ADDRESS)
    if device is None:
        logger.error("could not find device with address '%s'", PHONE_ADDRESS)
        return
    # else:
    #     device = await BleakScanner.find_device_by_name(
    #         args.name, cb=dict(use_bdaddr=args.macos_use_bdaddr)
    #     )
    #     if device is None:
    #         logger.error("could not find device with name '%s'", args.name)
    #         return

    logger.info("connecting to device...")

    async with BleakClient(device) as client:
        logger.info("Connected")

        await client.start_notify(ENTITY_UPDATE_UUID, notification_handler)
        await client.write_gatt_char(ENTITY_UPDATE_UUID, bytearray([0x02, 0x00, 0x01, 0x02, 0x03]))
        await client.write_gatt_char(ENTITY_UPDATE_UUID, bytearray([0x00, 0x00, 0x01, 0x02]))
        await client.write_gatt_char(ENTITY_UPDATE_UUID, bytearray([0x01, 0x00]))

        while (client.is_connected):
            await asyncio.sleep(1.0)
        # await asyncio.sleep(50.0)
        await client.stop_notify(ENTITY_UPDATE_UUID)


if __name__ == "__main__":

    log_level = logging.DEBUG if False else logging.INFO
    logging.basicConfig(
        level=log_level,
        format="%(asctime)-15s %(name)-8s %(levelname)s: %(message)s",
    )

    asyncio.run(main())