import subprocess
import dbus
import dbus.service
try:
  from gi.repository import GLib
except ImportError:
  import gobject as GObject
import sys
from enum import Enum
from dbus.mainloop.glib import DBusGMainLoop
import os
import json
import datetime
import time
import threading
from collections.abc import MutableMapping


bus = None
mainloop = None
buss = None
busself = None
ams_dbus = None



AMS_OPATH = "/dev/anjay/ams"
AMS_IFACE = "dev.anjay.ams"
AMS_BUS_NAME = "dev.anjay.ams"

BLUEZ_SERVICE_NAME = 'org.bluez'
DBUS_OM_IFACE =      'org.freedesktop.DBus.ObjectManager'
DBUS_PROP_IFACE =    'org.freedesktop.DBus.Properties'

GATT_SERVICE_IFACE = 'org.bluez.GattService1'
GATT_CHRC_IFACE =    'org.bluez.GattCharacteristic1'

# AMS
AMS_UUID =             '89D3502B-0F36-433A-8EF4-C502AD55F8DC' # Apple Media Service
REMOTE_COMMAND_UUID =  '9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2' # Remote Command
ENTITY_UPDATE_UUID =   '2F7CABCE-808D-411F-9A0C-BB92BA96C102' # Entity update
ENTITY_ATTR_UUID =     'C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7' # Entity attribute

# ams objects.
ams_service = None
ams_rc_chrc = None
ams_ent_upd_chrc = None
ams_ent_attr_chrc = None

class ENTITY_ATTR:
    def __init__(self, data):
        self.EntityID = int(data[0])
        self.AttributeID = int(data[1])
        self.EntityUpdateFlags = int(data[2])
        self.Value = bytes(data[3:]).decode()#''.join([str(v) for v in data[3:]])
        global Entity
        Entity['Connected']['Connected'] = True

# class ENTITY:
#     def __init__(self, data):
#         self.EntityID = int(data[0])
#         self.EntityType = int(data[1])
#         self.EntityCategory = int(data[2])
#         self.EntityNameSpace = int(data[3])
#         self.EntityDescription = ''.join([str(v) for v in data[4:]])

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
        'QueueAttributeIDCount': None,
        'QueueAttributeIDShuffleMode': None,
        'QueueAttributeIDRepeatMode': None
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
        'Connected': False
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


crumbs = True
def flatten(dictionary, parent_key=False, separator='.'):
    """
    Turn a nested dictionary into a flattened dictionary
    :param dictionary: The dictionary to flatten
    :param parent_key: The string to prepend to dictionary's keys
    :param separator: The string used to separate flattened keys
    :return: A flattened dictionary
    """

    items = []
    for key, value in dictionary.items():
        if crumbs: print('checking:',key)
        new_key = str(parent_key) + separator + key if parent_key else key
        if isinstance(value, MutableMapping):
            if crumbs: print(new_key,': dict found')
            if not value.items():
                if crumbs: print('Adding key-value pair:',new_key,None)
                items.append((new_key,None))
            else:
                items.extend(flatten(value, new_key, separator).items())
        elif isinstance(value, list):
            if crumbs: print(new_key,': list found')
            if len(value):
                for k, v in enumerate(value):
                    items.extend(flatten({str(k): v}, new_key, separator).items())
            else:
                if crumbs: print('Adding key-value pair:',new_key,None)
                items.append((new_key,None))
        else:
            if crumbs: print('Adding key-value pair:',new_key,value)
            items.append((new_key, value))
    return dict(items)

def generic_error_cb(error):
    print('D-Bus call failed: ' + str(error))
    # mainloop.quit()
    return False

def ams_ent_upd_start_notify_cb():
    return
    # print('AMS Entity Update notifications enabled')

def hr_msrmt_changed_cb(iface, changed_props, invalidated_props):
    if iface != GATT_CHRC_IFACE:
        return

    if not len(changed_props):
        return

    value = changed_props.get('Value', None)
    if not value:
        return

    print('New HR Measurement')

    flags = value[0]
    value_format = flags & 0x01
    sc_status = (flags >> 1) & 0x03
    ee_status = flags & 0x08

    if value_format == 0x00:
        hr_msrmt = value[1]
        next_ind = 2
    else:
        hr_msrmt = value[1] | (value[2] << 8)
        next_ind = 3

    print('\tHR: ' + str(int(hr_msrmt)))
    print('\tSensor Contact status: ' +
          sensor_contact_val_to_str(sc_status))

    if ee_status:
        print('\tEnergy Expended: ' + str(int(value[next_ind])))

def ams_ent_attr_changed_cb(iface, changed_props, invalidated_props):
    if iface != GATT_CHRC_IFACE:
        return

    if not len(changed_props):
        return

    value = changed_props.get('Value', None)
    if not value:
        return
    
    # print('New AMS Track')

    value = ENTITY_ATTR(value)
    # print(value.Value)
    one = list(Entity.keys())[value.EntityID]
    two = list(Entity[one].keys())[value.AttributeID]
    Entity[one][two] = value.Value
    print(str(datetime.datetime.now()))
    Entity['ReciveTime']['ReciveTime'] = str(datetime.datetime.now())
    # print(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'])
    if (one == 'EntityIDPlayer') and (two == 'PlayerAttributeIDPlaybackInfo'):
        Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'] = tuple(map(float, str(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo']).split(',')))
    
    # if not two:
    #     key = one
    # else:
    #     key = two
    ams_dbus.Set(AMS_IFACE, f"{one}.{two}", Entity[one][two])
    writeOut()



def start_client():
    print('Starting client...')
    global Entity
    # Read the Body Sensor Location value and print it asynchronously.
    # body_snsr_loc_chrc[0].ReadValue({}, reply_handler=body_sensor_val_cb,
    #                                 error_handler=generic_error_cb,
    #                                 dbus_interface=GATT_CHRC_IFACE)

    # # Listen to PropertiesChanged signals from the Heart Measurement
    # # Characteristic.
    # hr_msrmt_prop_iface = dbus.Interface(hr_msrmt_chrc[0], DBUS_PROP_IFACE)
    # hr_msrmt_prop_iface.connect_to_signal("PropertiesChanged",
    #                                       hr_msrmt_changed_cb)
    ams_ent_upd_prop_iface = dbus.Interface(ams_ent_upd_chrc[0], DBUS_PROP_IFACE)
    ams_ent_upd_prop_iface.connect_to_signal("PropertiesChanged", ams_ent_attr_changed_cb)
    # # Subscribe to Heart Rate Measurement notifications.
    # hr_msrmt_chrc[0].StartNotify(reply_handler=hr_msrmt_start_notify_cb,
    #                              error_handler=generic_error_cb,
    #                              dbus_interface=GATT_CHRC_IFACE)
    while (Entity['Connected']['Connected'] == False):
        print('tryingagsin')
        time.sleep(2)

        try:
            ams_ent_upd_chrc[0].StartNotify(reply_handler=ams_ent_upd_start_notify_cb,
                                            error_handler=generic_error_cb,
                                            dbus_interface=GATT_CHRC_IFACE)
            ams_ent_upd_chrc[0].WriteValue([0x00, 0x00, 0x01, 0x02], {}, dbus_interface=GATT_CHRC_IFACE)
            ams_ent_upd_chrc[0].WriteValue([0x01, 0x00, 0x01, 0x02, 0x03], {}, dbus_interface=GATT_CHRC_IFACE)
            ams_ent_upd_chrc[0].WriteValue([0x02, 0x00, 0x01, 0x02, 0x03], {}, dbus_interface=GATT_CHRC_IFACE)
            Entity['Connected']['Connected'] = True
        except Exception as error:
            print(error, "tree")
        else:
            Entity['Connected']['Connected'] = True

    return True
    # t1 = threading.Thread(target=writeOut, args=(True,))
    # t1.daemon = True
    # t1.start()
    



def process_chrc(chrc_path):
    chrc = bus.get_object(BLUEZ_SERVICE_NAME, chrc_path)
    chrc_props = chrc.GetAll(GATT_CHRC_IFACE,
                             dbus_interface=DBUS_PROP_IFACE)

    uuid = chrc_props['UUID']

    if uuid == REMOTE_COMMAND_UUID.lower():
        global ams_rc_chrc
        ams_rc_chrc = (chrc, chrc_props)
    elif uuid == ENTITY_UPDATE_UUID.lower():
        global ams_ent_upd_chrc
        ams_ent_upd_chrc = (chrc, chrc_props)
    elif uuid == ENTITY_ATTR_UUID.lower():
        global ams_ent_attr_chrc
        ams_ent_attr_chrc = (chrc, chrc_props)
    else:
        print('Unrecognized characteristic: ' + uuid)

    return True


def process_ams_service(service_path, chrc_paths):
    service = bus.get_object(BLUEZ_SERVICE_NAME, service_path)
    service_props = service.GetAll(GATT_SERVICE_IFACE,
                                   dbus_interface=DBUS_PROP_IFACE)
    uuid = service_props['UUID']
    if uuid != AMS_UUID.lower():
        return False

    # print('AMS found: ' + service_path)

    # Process the characteristics.
    for chrc_path in chrc_paths:
        process_chrc(chrc_path)

    global ams_service
    ams_service = (service, service_props, service_path)

    return True


def interfaces_removed_cb(object_path, interfaces):
    print('remove')
    global mainloop
    global Entity
    Entity['Connected']['Connected'] = False
    writeOut()

    if not ams_service:
        print('No AMS Service found')
        mainloop.quit()
        global buss
        global busself
        dbus.service.Object.remove_from_connection(busself, buss)
        main()
        return

    if object_path == '/org/bluez/hci0/dev_B4_56_E3_B8_76_DA':
        print('Service was removed')
        start_client()
        return
        # global buss
        # global busself
        # dbus.service.Object.remove_from_connection(busself, buss)
        # mainloop.quit()
        # main()


        # GLib.source_remove(0)
        # subprocess.run(["bluetoothctl"])
        # subprocess.run(["advertise on"])
        # subprocess.run(["exit"])

def interfaces_added_cb(object_path, interfaces):
    print('add')
    if not ams_service:
        print('No AMS Service found')
        # time.sleep(2)
        mainloop.quit()
        main()
        return
    else:
        return
    # chrcs = []

    # if GATT_SERVICE_IFACE not in interfaces.keys():
    #     print('No GATT Service')
    #     return

    # chrc_paths = [d for d in chrcs if d.startswith(object_path + "/")]
    # # print(chrc_paths)
    # if process_ams_service(object_path, chrc_paths):
    start_client()
    # time.sleep(2)
    # mainloop.quit()
    # main()

    # if interfaces['org.bluez.GattCharacteristic1']['UUID'] == ENTITY_UPDATE_UUID.lower():
    #     # mainloop.quit()
    #     start_client()


def main():
    # Set up the main loop.
    print('Starting')

    # return
    DBusGMainLoop(set_as_default=True)
    global bus
    bus = dbus.SystemBus()
    global mainloop
    mainloop = GLib.MainLoop()

    om = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, '/'), DBUS_OM_IFACE)
    om.connect_to_signal('InterfacesRemoved', interfaces_removed_cb)
    om.connect_to_signal('InterfacesAdded', interfaces_added_cb)

    # print('Getting objects...')
    objects = om.GetManagedObjects()
    # print(objects)
    # for item in objects:
    #     print(item + '\n')
    chrcs = []
    # List characteristics found
    for path, interfaces in objects.items():
        if GATT_CHRC_IFACE not in interfaces.keys():
            continue
        chrcs.append(path)
    # List sevices found
    for path, interfaces in objects.items():
        if GATT_SERVICE_IFACE not in interfaces.keys():
            continue

        chrc_paths = [d for d in chrcs if d.startswith(path + "/")]
        # print(chrc_paths)
        if process_ams_service(path, chrc_paths):
            break

    if not ams_service:
        print('No AMS Service found')
        time.sleep(2)
        main()
        # sys.exit(1)

    start_client()
    # try:
    global ams_dbus
    ams_dbus = Command()
    # except:
    #     time.sleep(2)
    #     start_client
    GLib.timeout_add_seconds(1, writeOut)

    mainloop.run()


class Command(dbus.service.Object):
    def __init__(self):
        global buss
        global busself
        global Entity
        busself = self

        self.ams_properties = Entity

        buss = dbus.SessionBus()
        buss.request_name(AMS_BUS_NAME)
        bus_name = dbus.service.BusName(AMS_BUS_NAME, bus=buss)
        dbus.service.Object.__init__(self, bus_name, AMS_OPATH)

    @dbus.service.method(dbus_interface=AMS_IFACE, in_signature="", out_signature="")
    def amsBack(self):
        ams_rc_chrc[0].WriteValue([0x04], {}, dbus_interface=GATT_CHRC_IFACE)
        return True

    @dbus.service.method(dbus_interface=AMS_IFACE, in_signature="", out_signature="")
    def amsForward(self):
        ams_rc_chrc[0].WriteValue([0x03], {}, dbus_interface=GATT_CHRC_IFACE)
        return True

    @dbus.service.method(dbus_interface=AMS_IFACE, in_signature="", out_signature="")
    def amsToggle(self):
        ams_rc_chrc[0].WriteValue([0x02], {}, dbus_interface=GATT_CHRC_IFACE)
        return True
    
    @dbus.service.method(dbus_interface=AMS_IFACE, in_signature="", out_signature="")
    def amsVolUp(self):
        ams_rc_chrc[0].WriteValue([0x05], {}, dbus_interface=GATT_CHRC_IFACE)
        return True
    
    @dbus.service.method(dbus_interface=AMS_IFACE, in_signature="", out_signature="")
    def amsVolDn(self):
        ams_rc_chrc[0].WriteValue([0x06], {}, dbus_interface=GATT_CHRC_IFACE)
        return True
    
    @dbus.service.method(dbus_interface=AMS_IFACE, in_signature="", out_signature="")
    def advanceRepeat(self):
        ams_rc_chrc[0].WriteValue([0x07], {}, dbus_interface=GATT_CHRC_IFACE)
        return True
    



    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE, in_signature='ss', out_signature='v')
    def Get(self, interface_name, property_name):
        return self.GetAll(interface_name)[property_name]

    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE, in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface_name):
        # global mpris_properties
        # global mpris_player_properties
        if interface_name == AMS_IFACE:
            return flatten(self.ams_properties)
        else:
            raise dbus.exceptions.DBusException('com.example.UnknownInterface', 'The Foo object does not implement the %s interface' % interface_name)



    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE, in_signature='ssv')
    def Set(self, interface_name, property_name, new_value):
        # validate the property name and value, update internal state…
        # print(entity)
        self.PropertiesChanged(interface_name, { property_name: new_value }, [])

    @dbus.service.signal(dbus_interface=dbus.PROPERTIES_IFACE, signature='sa{sv}as')
    def PropertiesChanged(self, interface_name, changed_properties, invalidated_properties):
        pass# return (changed_properties)

if __name__ == '__main__':
    main()

