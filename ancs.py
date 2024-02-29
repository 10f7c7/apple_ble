import subprocess
import dbus
import dbus.service
import requests
try:
  from gi.repository import GLib
except ImportError:
  import gobject as GObject
import sys
from enum import Enum, unique, Flag
from dbus.mainloop.glib import DBusGMainLoop
import os
import json
import datetime
import time
import threading
import hashlib


bus = None
mainloop = None
connected = False
overflow = None

ANCS_OPATH = "/dev/anjay/ancs"
ANCS_IFACE = "dev.anjay.ancs"
ANCS_BUS_NAME = "dev.anjay.ancs"

BLUEZ_SERVICE_NAME = 'org.bluez'
DBUS_OM_IFACE =      'org.freedesktop.DBus.ObjectManager'
DBUS_PROP_IFACE =    'org.freedesktop.DBus.Properties'

GATT_SERVICE_IFACE = 'org.bluez.GattService1'
GATT_CHRC_IFACE =    'org.bluez.GattCharacteristic1'

# ANCS
ANCS_UUID = '7905F431-B5CE-4E99-A40F-4B1E122D00D0' # Apple Notification Center Service
ANCS_NOTIF_SRC_UUID = '9FBF120D-6301-42D9-8C58-25E699A21DBD' # Notification Source
ANCS_CTRL_PT_UUID = '69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9' # Control Point
ANCS_DATA_SRC_UUID = '22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB' # Data Source

# ancs objects
ancs_service = None
ancs_notif_src_chrc = None
ancs_ctrl_pt_chrc = None
ancs_data_src_chrc = None

notif_index = []

appli_index = {}


@unique
class NOTIF_ATTR(Enum):
    AppIdentifier = 0x00
    Title = 0x01
    Subtitle = 0x02
    Message = 0x03
    MessageSize = 0x04
    Date = 0x05
    PositiveActionLabel = 0x06
    NegativeActionLabel = 0x07

@unique
class NOTIF_FLAG(Enum):
    Silent = 0x00
    Important = 0x01
    PreExisting = 0x02
    PositiveAction = 0x03
    NegativeAction = 0x04


class NOTIF_ATTR2:
    def __init__(self, flags):
        self.values = ['AppIdentifier', 'Title', 'Subtitle', 'Message', 'MessageSize', 'Date', 'PositiveActionLabel', 'NegativeActionLabel']
        self.hex = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07]
        self.completed = [False, False, False, False, False, False, False, False]
        self.flags = flags
        self.j = 0
        self.name = self.values[self.j]
        self.done = False
    def next(self):
        self.completed[self.j] = True
        if all(self.completed):
            self.done = True
            return
        self.j += 1
        if (self.j == 6) and not (self.flags & (1 << NOTIF_FLAG.PositiveAction.value)):
            self.completed[self.j] = True
            self.j += 1
        if (self.j == 7) and not (self.flags & (1 << NOTIF_FLAG.NegativeAction.value)):
            self.completed[self.j] = True
            # self.j += 1
        self.name = self.values[self.j]
        return self
    

class APPLI_ATTR:
    def __init__(self):
        self.values = ['DisplayName']
        self.hex = [0x00]
        self.completed = [False]
        self.j = 0
        self.name = self.values[self.j]
        self.done = False
    def next(self):
        self.completed[self.j] = True
        if all(self.completed):
            self.done = True
            return
        self.j += 1
        self.name = self.values[self.j]
        return self


class NOTIF_SRC:
    def __init__(self, data):
        self.EventID = int(data[0])
        self.EventFlags = int(data[1])
        self.CategoryID = int(data[2])
        self.CategoryCount = int(data[3])
        self.NotificationUID = [data[4], data[5], data[6], data[7]]#''.join([str(v) for v in data[3:]])
        self.NotificationUIDDec = int(data[4]) + (int(data[5])*256) + (int(data[6])*65536) + (int(data[7])*16777216)
        # self.Value = ''.join([str(v) for v in data[3:]])


def int_to_bytes(var):
    bin = '{:032b}'.format(var)
    notifid = [bin[i:i+8] for i in range(0, len(bin), 8)]
    # print(*notifid, sep = ", ")
    notifid = list(map(tohex, notifid))
    # notifid = [0x00, 0x00, 0x00, 0x00]
    notifid = notifid[::-1]
    return notifid


class DATA_SRC:
    def __init__(self, data):
        incomdata = data
        global overflow
        if overflow:
            data = [*overflow, *data]
        of = False
        eh = False
        i = 0
        self.data = {}
        self.data['CommandID'] = int(data[i])
        i += 1
        if self.data['CommandID'] == 0:
            self.data['NotificationUID'] = int(data[i]) + (int(data[i+1])*256) + (int(data[i+2])*65536) + (int(data[i+3])*16777216)
            i += 4
            flags = notif_index[self.data['NotificationUID']]
        if self.data['CommandID'] == 1:
            george = []
            while data[i] != 0x00:
                george.append(data[i])
                i += 1
            i += 1
            self.data['AppIdentifier'] = bytes(george).decode()
        
        # self.AppIdentifier = [data[i+1], data[i+2]]
        # print(''.join([str(v) for v in data[i:(i+int(data[i+1])+int(data[i+2]))]]))
        # it = iter(NOTIF_ATTR)
        # print (len(data))
        if self.data['CommandID'] == 0:
            cur = NOTIF_ATTR2(flags)
        
        if self.data['CommandID'] == 1:
            cur = APPLI_ATTR()

        # print(hex(data[i]))
        while (i < len(data)) and not of:
            if of:
                print("of" + i)
            # print (hex(int(data[i])))
            if data[i] == cur.hex[cur.j]:
                eh = False
                # print(cur)
                # print(hex(int(data[i])))
                # print(hex(int(data[i+1])) + " , " + hex(int(data[i+2])))
                # i += 1
                try:
                    length = int(data[i+1])+(int(data[i+2])*256)
                except:
                    of = True
                    print('Overflowlenerr')
                    overflow = [*overflow, *incomdata]
                    break
                # going over id and length
                i += 3
                # print(str(i+length) + ",,," + str(len(data)))
                if (i+length) > len(data):
                    of = True
                    print('Overflowtoomuch')
                    overflow = [*overflow, *incomdata]
                    break
                self.data[cur.name] = bytes(data[i:(i+length)]).decode()
                # self.data[cur.name] = ''.join([chr(v) for v in data[i:(i+length)]])
                i += length
                cur.next()
                # print(all(cur.completed))
                if cur.done:
                    break
                eh = True
                # cur = next(it)
                if of:
                    print('of')
                # print("eugh" + str(len(data)-i))
            else:
                # self.data[NOTIF_ATTR(cur.value-1).name] += chr(data[i])
                # print(hex(data[i]))
                raise ValueError('Invalid data')
                # print(i)
                # i += 1
                # print(self.data['NotificationUID'])
                # print(hex(int(data[i-3])), hex(int(data[i-2])), hex(int(data[i-1])), hex(int(data[i])), hex(int(data[i+1])), hex(int(data[i+2])))
                # print(cur.name)
        if eh:
            of = True
            print('Overflow but out to hurt you')
            overflow = [*overflow, *incomdata]
            # print(''.join([chr(v) for v in overflow]))
        if overflow and not of:
            print('Overflow over')
            overflow = []
            # print(''.join([chr(v) for v in data]))
            # print('-')




# class ENTITY:
#     def __init__(self, data):
#         self.EntityID = int(data[0])
#         self.EntityType = int(data[1])
#         self.EntityCategory = int(data[2])
#         self.EntityNameSpace = int(data[3])
#         self.EntityDescription = ''.join([str(v) for v in data[4:]])


def write_notification(value):
    global notif_index
    action = []
    if (notif_index[value.data['NotificationUID']] & (1 << NOTIF_FLAG.PositiveAction.value)):
        action.append(f"1634624371 0 {value.data['NotificationUID']}")
        action.append(value.data['PositiveActionLabel'])
    if (notif_index[value.data['NotificationUID']] & (1 << NOTIF_FLAG.NegativeAction.value)):
        action.append(f"1634624371 1 {value.data['NotificationUID']}")
        action.append(value.data['NegativeActionLabel'])


    if value.data['AppIdentifier'] in appli_index:
        appli_name = appli_index[value.data['AppIdentifier']][0]
    else:
        appinfo = requests.get(f"https://itunes.apple.com/lookup/?bundleId={value.data['AppIdentifier']}").json()
        if appinfo['resultCount'] == 0:
            appli_name = "Rats"
        else:
            appli_name = appinfo['results'][0]['trackName']
            filename = f"/home/10f7c7/Projects/apple_ble/app_icons/{value.data['AppIdentifier']}.{appinfo['results'][0]['artworkUrl60'].split('.')[-1]}"
            r = requests.get(appinfo['results'][0]['artworkUrl60'], stream=True)
            if os.path.isfile(filename):
                hash_online = hashlib.md5(r.raw.read()).hexdigest()
                has_download = hashlib.md5(open(filename,'rb').read()).hexdigest()

            if not os.path.isfile(filename):# or (hash_online != has_download):
                with open(filename, 'wb') as fd:
                    for chunk in r.iter_content(chunk_size=128):
                        fd.write(chunk)
            appli_index[value.data['AppIdentifier']] = [appinfo['results'][0]['trackName'], filename]

            appli_index[value.data['AppIdentifier']].append(appinfo['results'][0]['trackName'])
        # ancs_ctrl_pt_chrc[0].WriteValue([0x01, *value.data['AppIdentifier'].encode('utf-8'), 0x00, 0x00], {}, dbus_interface=GATT_CHRC_IFACE)
        # appli_name = "Rats"
        #appli_name = appli_index[value.data['AppIdentifier']]

    obj = dbus.SessionBus().get_object("org.freedesktop.Notifications", "/org/freedesktop/Notifications")
    obj = dbus.Interface(obj, "org.freedesktop.Notifications")
    obj.Notify(appli_name, 0, appli_index[value.data['AppIdentifier']][1], value.data['Title'], value.data['Message'], action, {"urgency": 1, "ancs": True}, 10000)




def generic_error_cb(error):
    print('D-Bus call failed: ' + str(error))
    mainloop.quit()

def ancs_notif_src_start_notify_cb():
    return
    # print('AMS Entity Update notifications enabled')

def ancs_data_src_start_notify_cb():
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

def ancs_notif_src_changed_cb(iface, changed_props, invalidated_props):
    if iface != GATT_CHRC_IFACE:
        return

    if not len(changed_props):
        return

    value = changed_props.get('Value', None)
    if not value:
        return
    
    # print('New ANCS notif')
    value = NOTIF_SRC(value)
    # print(*value.NotificationUID)
    # print(value.NotificationUIDDec)
    notif_index.insert(value.NotificationUIDDec, value.EventFlags)
    if value.EventID == 0x02:
        return
    # preexist = (value.EventFlags >> 1) & 0x02
    preexist = value.EventFlags & (1 << NOTIF_FLAG.PreExisting.value)
    
    action = []

    if (value.EventFlags & (1 << NOTIF_FLAG.PositiveAction.value)):
        action.append(0x06)
    if (value.EventFlags & (1 << NOTIF_FLAG.NegativeAction.value)):
        action.append(0x07)
    # preexist = value.EventFlags & 0x02
    # if preexist:
    # print(value.EventID)
    # print(value.NotificationUID)
    if not preexist:
        print(value.NotificationUID)
    ancs_ctrl_pt_chrc[0].WriteValue([0x00, *value.NotificationUID, 0x00, 0x01, 0xff, 0xff, 0x02, 0xff, 0xff, 0x03, 0xff, 0xff, 0x04, 0x05, *action], {}, dbus_interface=GATT_CHRC_IFACE)
        # ancs_ctrl_pt_chrc[0].WriteValue([0x00, *value.NotificationUID, 0x04], {}, dbus_interface=GATT_CHRC_IFACE)

    # value = ENTITY_ATTR(value)
    # # print(value.Value)
    # one = list(Entity.keys())[value.EntityID]
    # two = list(Entity[one].keys())[value.AttributeID]
    # Entity[one][two] = value.Value
    # Entity['ReciveTime'] = datetime.datetime.now()
    # # print(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'])
    # if (one == 'EntityIDPlayer') and (two == 'PlayerAttributeIDPlaybackInfo'):
    #     Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo'] = tuple(map(float, str(Entity['EntityIDPlayer']['PlayerAttributeIDPlaybackInfo']).split(',')))
    # writeOut()

def ancs_data_src_changed_cb(iface, changed_props, invalidated_props):
    global overflow
    global appli_index
    if iface != GATT_CHRC_IFACE:
        return

    if not len(changed_props):
        return

    value = changed_props.get('Value', None)
    if not value:
        return
    # print(len(value))
    # if (len(value) == 182):# and (value[0] < 0x10):
    #     # print('Overflow')
    #     global overflow
    #     overflow = [*overflow, *value]
    #     return
    
    # # if value[0] > 0x10 and overflow:
    # if (len(value) < 182) and overflow:
    #     # print('Overflow over')
    #     value = [*overflow, *value]
    #     overflow = []

    value = DATA_SRC(value)

    if value.data['CommandID'] == 0:
        if not overflow:
            print(value.data['Title'])
            print(value.data['Message'])
            print(value.data['NotificationUID'])
            print('\n')

            write_notification(value)
            #value.data['AppIdentifier']
    
    if value.data['CommandID'] == 1:
        appli_index[value.data['AppIdentifier']] = value.data['DisplayName']
        print("NAME: ", appli_index[value.data['AppIdentifier']])



def start_client():
    global connected
    # Read the Body Sensor Location value and print it asynchronously.
    # body_snsr_loc_chrc[0].ReadValue({}, reply_handler=body_sensor_val_cb,
    #                                 error_handler=generic_error_cb,
    #                                 dbus_interface=GATT_CHRC_IFACE)

    # # Listen to PropertiesChanged signals from the Heart Measurement
    # # Characteristic.
    # hr_msrmt_prop_iface = dbus.Interface(hr_msrmt_chrc[0], DBUS_PROP_IFACE)
    # hr_msrmt_prop_iface.connect_to_signal("PropertiesChanged",
    #                                       hr_msrmt_changed_cb)
    connected = True
    ancs_notif_src_prop_iface = dbus.Interface(ancs_notif_src_chrc[0], DBUS_PROP_IFACE)
    ancs_notif_src_prop_iface.connect_to_signal("PropertiesChanged", ancs_notif_src_changed_cb)
    ancs_data_src_prop_iface = dbus.Interface(ancs_data_src_chrc[0], DBUS_PROP_IFACE)
    ancs_data_src_prop_iface.connect_to_signal("PropertiesChanged", ancs_data_src_changed_cb)
    # # Subscribe to Heart Rate Measurement notifications.
    # hr_msrmt_chrc[0].StartNotify(reply_handler=hr_msrmt_start_notify_cb,
    #                              error_handler=generic_error_cb,
    #                              dbus_interface=GATT_CHRC_IFACE)
    ancs_notif_src_chrc[0].StartNotify(reply_handler=ancs_notif_src_start_notify_cb,
                                    error_handler=generic_error_cb,
                                    dbus_interface=GATT_CHRC_IFACE)
    ancs_data_src_chrc[0].StartNotify(reply_handler=ancs_data_src_start_notify_cb,
                                    error_handler=generic_error_cb,
                                    dbus_interface=GATT_CHRC_IFACE)
    # ancs_notif_src_chrc[0].WriteValue([0x00, 0x00, 0x01, 0x02], {}, dbus_interface=GATT_CHRC_IFACE)
    # ancs_notif_src_chrc[0].WriteValue([0x01, 0x00, 0x01, 0x02, 0x03], {}, dbus_interface=GATT_CHRC_IFACE)
    # ancs_notif_src_chrc[0].WriteValue([0x02, 0x00, 0x01, 0x02, 0x03], {}, dbus_interface=GATT_CHRC_IFACE)
    #fGLib.timeout_add_seconds(1, writeOut)
    # t1 = threading.Thread(target=writeOut, args=(True,))
    # t1.daemon = True
    # t1.start()
    



def process_chrc(chrc_path):
    chrc = bus.get_object(BLUEZ_SERVICE_NAME, chrc_path)
    chrc_props = chrc.GetAll(GATT_CHRC_IFACE,
                             dbus_interface=DBUS_PROP_IFACE)

    uuid = chrc_props['UUID']

    if uuid == ANCS_NOTIF_SRC_UUID.lower():
        global ancs_notif_src_chrc
        ancs_notif_src_chrc = (chrc, chrc_props)
    elif uuid == ANCS_CTRL_PT_UUID.lower():
        global ancs_ctrl_pt_chrc
        ancs_ctrl_pt_chrc = (chrc, chrc_props)
    elif uuid == ANCS_DATA_SRC_UUID.lower():
        global ancs_data_src_chrc
        ancs_data_src_chrc = (chrc, chrc_props)
    else:
        print('Unrecognized characteristic: ' + uuid)

    return True


def process_ancs_service(service_path, chrc_paths):
    service = bus.get_object(BLUEZ_SERVICE_NAME, service_path)
    service_props = service.GetAll(GATT_SERVICE_IFACE,
                                   dbus_interface=DBUS_PROP_IFACE)

    uuid = service_props['UUID']
    if uuid != ANCS_UUID.lower():
        return False

    # print('AMS found: ' + service_path)

    # Process the characteristics.
    for chrc_path in chrc_paths:
        process_chrc(chrc_path)

    global ancs_service
    ancs_service = (service, service_props, service_path)

    return True


def interfaces_removed_cb(object_path, interfaces):
    global connected
    if not ancs_service:
        return

    if object_path == '/org/bluez/hci0/dev_B4_56_E3_B8_76_DA':
        print('Service was removed')
        connected = False
        # GLib.source_remove(0)
        # subprocess.run(["bluetoothctl"])
        # subprocess.run(["advertise on"])
        # subprocess.run(["exit"])

def interfaces_added_cb(object_path, interfaces):
    if not ancs_service:
        return

    if interfaces['org.bluez.GattCharacteristic1']['UUID'] == ENTITY_UPDATE_UUID.lower():
        # mainloop.quit()
        start_client()

def noti_cb(id, action):
    print('Noti')
    # print(id, action)
    ancs_id, noti_action, noti_id = map(lambda a: int(a), action.split())

    if ancs_id != 1634624371:
        return

    bin = '{:032b}'.format(noti_id)
    notifid = [bin[i:i+8] for i in range(0, len(bin), 8)]
    notifid = list(map(tohex, notifid))
    notifid = notifid[::-1]
    if noti_action == 0:
        ancs_ctrl_pt_chrc[0].WriteValue([0x02, *notifid, 0x00], {}, dbus_interface=GATT_CHRC_IFACE)
    
    if noti_action == 1:
        ancs_ctrl_pt_chrc[0].WriteValue([0x02, *notifid, 0x01], {}, dbus_interface=GATT_CHRC_IFACE)

def main():
    # Set up the main loop.
    print('Starting')
    
    # return
    DBusGMainLoop(set_as_default=True)
    noti = dbus.SessionBus().get_object("org.freedesktop.Notifications", "/org/freedesktop/Notifications")
    noti = dbus.Interface(noti, "org.freedesktop.Notifications")

    noti.connect_to_signal("ActionInvoked", noti_cb)

    global bus
    bus = dbus.SystemBus()
    global mainloop
    mainloop = GLib.MainLoop()
    global connected
    connected = False
    global overflow
    overflow = []

    om = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, '/'), DBUS_OM_IFACE)
    om.connect_to_signal('InterfacesRemoved', interfaces_removed_cb)
    om.connect_to_signal('InterfacesAdded', interfaces_added_cb)

    # print('Getting objects...')
    objects = om.GetManagedObjects()
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
        if process_ancs_service(path, chrc_paths):
            break

    if not ancs_service:
        print('No ANCS Service found')
        time.sleep(2)
        main()
        # sys.exit(1)

    start_client()
    a = Command()

    mainloop.run()


def tohex(val):
    return int(hex(int(val, 2)), 0)

class Command(dbus.service.Object):
    def __init__(self):
        buss = dbus.SessionBus()
        buss.request_name(ANCS_BUS_NAME)
        bus_name = dbus.service.BusName(ANCS_BUS_NAME, bus=buss)
        dbus.service.Object.__init__(self, bus_name, ANCS_OPATH)

    @dbus.service.method(dbus_interface=ANCS_IFACE, in_signature="s", out_signature="")
    def Clear(self, var):
        var = var-1
        # print(var)
        bin = '{:032b}'.format(var)
        print(bin)

        # notifid = list(map(''.join, zip(*[iter(bin)]*4))).reverse()
        notifid = [bin[i:i+8] for i in range(0, len(bin), 8)]
        # print(*notifid, sep = ", ")
        notifid = list(map(tohex, notifid))
        # notifid = [0x00, 0x00, 0x00, 0x00]
        notifid = notifid[::-1]
        # print(notifid)
        # print(*notifid,sep = ", ")
        # print(notifid)
        ancs_ctrl_pt_chrc[0].WriteValue([0x02, *notifid, 0x01], {}, dbus_interface=GATT_CHRC_IFACE)
        # ancs_ctrl_pt_chrc[0].WriteValue([0x04], {}, dbus_interface=GATT_CHRC_IFACE)
        return True


if __name__ == '__main__':
    main()