import subprocess
import dbus
import dbus.service
import requests
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


# dbus-send --session --dest=org.mpris.MediaPlayer2.ams --print-reply /org/mpris/MediaPlayer2 \
# org.freedesktop.DBus.Properties.Get string:org.mpris.MediaPlayer2.Player string:PlaybackStatus

AMS_MPRIS_OPATH = "/org/mpris/MediaPlayer2"
AMS_MPRIS_IFACE = "org.mpris.MediaPlayer2"
AMS_MPRIS_PLAYER_IFACE = "org.mpris.MediaPlayer2.Player"
AMS_MPRIS_BUS_NAME = "org.mpris.MediaPlayer2.ams"

DBUS_OM_IFACE =      'org.freedesktop.DBus.ObjectManager'
DBUS_PROP_IFACE =    'org.freedesktop.DBus.Properties'

buss = None
busself = None
mpris_dbus = None
amsbus = None

mpris_properties = {
    "Identity": "AMS",
}

mpris_player_properties = {
    "PlaybackStatus": "Playing",
    "Metadata": {
        "variant_level": dbus.types.String("map", variant_level=1),
        "mpris:artUrl": dbus.types.String("file:///home/10f7c7/Images/funntcat.jpg", variant_level=1),
        "xesam:title":"AMS",
        "mpris:trackid": dbus.types.ObjectPath("/org/mpris/MediaPlayer2/ams", variant_level=1),
        "mpris:length": dbus.types.Int64(1000000, variant_level=1),
    },
    "CanControl": True,
    "CanGoNext": True,
    "CanGoPrevious": True,
    "CanPause": True,
    "CanPlay": True,
    "CanSeek": False,
    # "LoopStatus": "None",
    "Rate": 1.0,
    # "Shuffle": False,
    "Volume": 1.0,
    "Position": dbus.types.Int64(0, variant_level=1),
}


def sstous(s):
    return int(float(s)*1000000)


class Command(dbus.service.Object):
    def __init__(self):
        global buss
        global busself
        busself = self
        self.mpris_properties = mpris_properties
        self.mpris_player_properties = mpris_player_properties
        
        buss = dbus.SessionBus()
        buss.request_name(AMS_MPRIS_BUS_NAME)
        bus_name = dbus.service.BusName(AMS_MPRIS_BUS_NAME, bus=buss)
        dbus.service.Object.__init__(self, bus_name, AMS_MPRIS_OPATH)

    @dbus.service.method(dbus_interface=AMS_MPRIS_IFACE, in_signature="", out_signature="")
    def Quit(self):
        sys.exit(0)
        return True

    @dbus.service.method(dbus_interface=AMS_MPRIS_PLAYER_IFACE, in_signature="", out_signature="")
    def PlayPause(self):
        print("PlayPause")
        amsbus.amsToggle()
        # if self.mpris_player_properties["PlaybackStatus"] == "Playing":
        #     self.mpris_player_properties["PlaybackStatus"] = "Paused"
        #     self.Set(AMS_MPRIS_PLAYER_IFACE, "PlaybackStatus", self.mpris_player_properties["PlaybackStatus"])
        #     print ("now Pausing")
        # elif self.mpris_player_properties["PlaybackStatus"] == "Paused":
        #     self.mpris_player_properties["PlaybackStatus"] = "Playing"
        #     self.Set(AMS_MPRIS_PLAYER_IFACE, "PlaybackStatus", self.mpris_player_properties["PlaybackStatus"])
        #     print ("now Playing")

        # self.Set(AMS_MPRIS_PLAYER_IFACE, "PlaybackStatus", self.mpris_player_properties["Playback_Status"])
        # # self.Set(AMS_MPRIS_PLAYER_IFACE, {"PlaybackStatus": self.mpris_player_properties["Playback_Status"]}, [])
        return True
    
    @dbus.service.method(dbus_interface=AMS_MPRIS_PLAYER_IFACE, in_signature="", out_signature="")
    def Pause(self):
        print("Pause")
        return
    
    @dbus.service.method(dbus_interface=AMS_MPRIS_PLAYER_IFACE, in_signature="", out_signature="")
    def Play(self):
        print("Play")
        return
    
    @dbus.service.method(dbus_interface=AMS_MPRIS_PLAYER_IFACE, in_signature="", out_signature="")
    def Next(self):
        print("Next")
        amsbus.amsForward()
        return
    
    @dbus.service.method(dbus_interface=AMS_MPRIS_PLAYER_IFACE, in_signature="", out_signature="")
    def Previous(self):
        print("Previous")
        amsbus.amsBack()
        return
    
    @dbus.service.method(dbus_interface=AMS_MPRIS_PLAYER_IFACE, in_signature="", out_signature="")
    def Stop(self):
        print("Stop")
        return
    
    @dbus.service.method(dbus_interface=AMS_MPRIS_PLAYER_IFACE, in_signature="x", out_signature="")
    def Seek(self, offset):
        print("Seek")
        return
    
    @dbus.service.method(dbus_interface=AMS_MPRIS_PLAYER_IFACE, in_signature="s", out_signature="")
    def SetPosition(self, track_id):
        print("SetPosition")
        return
    
    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE, in_signature='ss', out_signature='v')
    def Get(self, interface_name, property_name):
        return self.GetAll(interface_name)[property_name]

    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE, in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface_name):
        # global mpris_properties
        # global mpris_player_properties
        if interface_name == AMS_MPRIS_IFACE:
            return self.mpris_properties
        if interface_name == AMS_MPRIS_PLAYER_IFACE:
            return self.mpris_player_properties
        else:
            raise dbus.exceptions.DBusException('com.example.UnknownInterface', 'The Foo object does not implement the %s interface' % interface_name)

    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE, in_signature='ssv')
    def Set(self, interface_name, property_name, new_value):
        # validate the property name and value, update internal state…
        self.PropertiesChanged(interface_name, { property_name: new_value }, [])

    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE, in_signature='ss')
    def SetAll(self, interface_name, new_props):
        # validate the property name and value, update internal state…
        self.PropertiesChanged(interface_name, eval(new_props), [])

    @dbus.service.signal(dbus_interface=dbus.PROPERTIES_IFACE, signature='sa{sv}as')
    def PropertiesChanged(self, interface_name, changed_properties, invalidated_properties):
        # if interface_name == "org.mpris.MediaPlayer2.Player" and "LoopStatus" in changed_properties:
        #     print("LoopStatus changed to", changed_properties["LoopStatus"])
            # amsbus.advanceRepeat()
        pass#return changed_properties
    
def changed(iface, changed_props, invalidated_props):
    # print(iface)
    print(changed_props) 
    # print('\n')
    global mpris_player_properties
    global mpris_dbus
    current_mpris_data = None


    if changed_props.get('EntityIDTrack.TrackAttributeIDDuration'):
        current_mpris_data = 'Metadata'
        mpris_player_properties["Metadata"]["mpris:length"] = dbus.types.Int64(sstous(changed_props['EntityIDTrack.TrackAttributeIDDuration']))

    if changed_props.get('EntityIDTrack.TrackAttributeIDAlbum'):
        current_mpris_data = 'Metadata'
        mpris_player_properties["Metadata"]["xesam:album"] = changed_props['EntityIDTrack.TrackAttributeIDAlbum']

        term = mpris_player_properties["Metadata"]["xesam:album"].replace(" ", "+")
        filename = f"/home/10f7c7/Projects/apple_ble/album_icons/{term}"
        print(filename)
        if os.path.isfile(filename):
            mpris_player_properties["Metadata"]["mpris:artUrl"] = "file://" + filename
        else:

            albuminfo = requests.get(f"https://itunes.apple.com/search?media=music&entity=album&attribute=albumTerm&term={term}&limit=4").json()

            for album in albuminfo['results']:
                if mpris_player_properties["Metadata"]["xesam:artist"].find(",") or mpris_player_properties["Metadata"]["xesam:artist"].find("feat"):
                    vari = True
                if album['artistName'] == mpris_player_properties["Metadata"]["xesam:artist"] or vari:
                    if not os.path.isfile(filename):# or (hash_online != has_download):
                        with open(filename, 'wb') as fd:
                            r = requests.get(album['artworkUrl100']  , stream=True)
                            for chunk in r.iter_content(chunk_size=128):
                                fd.write(chunk)

                    mpris_player_properties["Metadata"]["mpris:artUrl"] = "file://" + filename
                    break
            if albuminfo['resultCount'] == 0:
                mpris_player_properties["Metadata"]["mpris:artUrl"] = "file:///home/10f7c7/Images/funntcat.jpg"


        




    if changed_props.get('EntityIDTrack.TrackAttributeIDArtist'):
        current_mpris_data = 'Metadata'
        mpris_player_properties["Metadata"]["xesam:artist"] = changed_props['EntityIDTrack.TrackAttributeIDArtist']

    if changed_props.get('EntityIDTrack.TrackAttributeIDTitle'):
        current_mpris_data = 'Metadata'
        mpris_player_properties["Metadata"]["xesam:title"] = changed_props['EntityIDTrack.TrackAttributeIDTitle']

    if changed_props.get('EntityIDTrack.TrackAttributeIDDuration'):
        current_mpris_data = 'Metadata'
        mpris_player_properties["Metadata"]["mpris:length"] = dbus.types.Int64(sstous(changed_props['EntityIDTrack.TrackAttributeIDDuration']))

    # if changed_props.get('EntityIDQueue.QueueAttributeIDRepeatMode'):
    #     current_mpris_data = 'LoopStatus'
    #     if changed_props['EntityIDQueue.QueueAttributeIDRepeatMode'] == 0:
    #         mpris_player_properties["LoopStatus"] = "None"
    #     elif changed_props['EntityIDQueue.QueueAttributeIDRepeatMode'] == 1:
    #         mpris_player_properties["LoopStatus"] = "Track"
    #     elif changed_props['EntityIDQueue.QueueAttributeIDRepeatMode'] == 2:
    #         mpris_player_properties["LoopStatus"] = "Playlist"
    # print(changed_props)
    if changed_props.get('EntityIDPlayer.PlayerAttributeIDPlaybackInfo'):
        current_mpris_data = 'PlaybackStatus'
        if changed_props['EntityIDPlayer.PlayerAttributeIDPlaybackInfo'][0] == 0:
            mpris_player_properties["PlaybackStatus"] = "Paused"
            print("Paused")
        elif changed_props['EntityIDPlayer.PlayerAttributeIDPlaybackInfo'][0] == 1:
            mpris_player_properties["PlaybackStatus"] = "Playing"
            print("Playing")
        mpris_dbus.Set(AMS_MPRIS_PLAYER_IFACE, current_mpris_data, mpris_player_properties[current_mpris_data])#str(repr(mpris_player_properties)))

        current_mpris_data = 'Rate'
        mpris_player_properties["Rate"] = changed_props['EntityIDPlayer.PlayerAttributeIDPlaybackInfo'][1]
        mpris_dbus.Set(AMS_MPRIS_PLAYER_IFACE, current_mpris_data, mpris_player_properties[current_mpris_data])#str(repr(mpris_player_properties)))
  


        current_mpris_data = 'Position'  
        mpris_player_properties["Position"] = dbus.types.Int64(sstous(changed_props['EntityIDPlayer.PlayerAttributeIDPlaybackInfo'][2]))

        mpris_dbus.Set(AMS_MPRIS_PLAYER_IFACE, current_mpris_data, mpris_player_properties[current_mpris_data])#str(repr(mpris_play  er_properties)))
        current_mpris_data = None
        return
  


    # if changed_props.get('EntityIDQueue.QueueAttributeIDShuffleMode'):    
    #     current_mpris_data = 'Shuffle'
    #     if changed_props['EntityIDQueue.QueueAttributeIDShuffleMode'] == 0:
    #         mpris_player_properties["Shuffle"] = False  
    #     elif changed_props['EntityIDQueue.QueueAttributeIDShuffleMode'] == 1 or changed_props['EntityIDQueue.QueueAttributeIDShuffleMode'] == 2:
    #         mpris_player_properties["Shuffle"] = True

    # if changed_props.get('EntityIDPlayer.PlayerAttributeIDVolume'):
    #     current_mpris_data = 'Volume'
    #     mpris_player_properties["Volume"] = changed_props['EntityIDPlayer.PlayerAttributeIDVolume']
    if changed_props.get("EntityIDQueue.QueueAttributeIDIndex"):
        current_mpris_data = 'Metadata'

    if (current_mpris_data):
        print ("current_mpris_data: ", current_mpris_data)
        print (changed_props)
        if current_mpris_data == "Metadata":
            if (changed_props.get("EntityIDQueue.QueueAttributeIDIndex") or changed_props.get("EntityIDTrack.TrackAttributeIDDuration")):
                print(mpris_player_properties)
                mpris_dbus.Set(AMS_MPRIS_PLAYER_IFACE, current_mpris_data, mpris_player_properties[current_mpris_data])#str(repr(mpris_player_properties)))
            else:
                return 
        else:
            mpris_dbus.Set(AMS_MPRIS_PLAYER_IFACE, current_mpris_data, mpris_player_properties[current_mpris_data])#str(repr(mpris_player_properties)))
    else:
        print ('\n')
        print("what happened: ", iface, " : ", changed_props, " : ",  invalidated_props)
        print ('\n')


def initGet():
    data = amsbus.GetAll("dev.anjay.ams")
    global mpris_player_properties
    mpris_player_properties["Metadata"]["mpris:length"] = dbus.types.Int64(sstous(data['EntityIDTrack.TrackAttributeIDDuration']))
    mpris_player_properties["Metadata"]["xesam:album"] = data['EntityIDTrack.TrackAttributeIDAlbum']
    mpris_player_properties["Metadata"]["xesam:artist"] = data['EntityIDTrack.TrackAttributeIDArtist']
    mpris_player_properties["Metadata"]["xesam:title"] = data['EntityIDTrack.TrackAttributeIDTitle']
    
    # if data['EntityIDQueue.QueueAttributeIDRepeatMode'] == 0:
    #     mpris_player_properties["LoopStatus"] = "None"
    # elif data['EntityIDQueue.QueueAttributeIDRepeatMode'] == 1:
    #     mpris_player_properties["LoopStatus"] = "Track"
    # elif data['EntityIDQueue.QueueAttributeIDRepeatMode'] == 2:
    #     mpris_player_properties["LoopStatus"] = "Playlist"
    
    if data['EntityIDPlayer.PlayerAttributeIDPlaybackInfo'][0] == 0:
        mpris_player_properties["PlaybackStatus"] = "Paused"
    else:
        mpris_player_properties["PlaybackStatus"] = "Playing"

    # if data['EntityIDQueue.QueueAttributeIDShuffleMode'] == 0:
    #     mpris_player_properties["Shuffle"] = False
    # elif data['EntityIDQueue.QueueAttributeIDShuffleMode'] == 1 or data['EntityIDQueue.QueueAttributeIDShuffleMode'] == 2:
    #     mpris_player_properties["Shuffle"] = True
    
    mpris_player_properties["Rate"] = data['EntityIDPlayer.PlayerAttributeIDPlaybackInfo'][2]

    mpris_player_properties["Position"] = dbus.types.Int64(sstous(data['EntityIDPlayer.PlayerAttributeIDPlaybackInfo'][2]))

    # mpris_player_properties["Volume"] = data['EntityIDPlayer.PlayerAttributeIDVolume']
    term = mpris_player_properties["Metadata"]["xesam:album"].replace(" ", "+")

    albuminfo = requests.get(f"https://itunes.apple.com/search?media=music&entity=album&attribute=albumTerm&term={term}&limit=4").json()
    filename = f"/home/10f7c7/Projects/apple_ble/album_icons/{term}"
    if os.path.isfile(filename):
        mpris_player_properties["Metadata"]["mpris:artUrl"] = "file://" + filename
    else:
        # print(f"https://itunes.apple.com/search?media=music&entity=album&attribute=albumTerm&term={term}&limit=4")

    # print(albuminfo)
        for album in albuminfo['results']:
            if mpris_player_properties["Metadata"]["xesam:artist"].find(",") or mpris_player_properties["Metadata"]["xesam:artist"].find("feat"):
                vari = True
            if album['artistName'] == mpris_player_properties["Metadata"]["xesam:artist"] or vari:
                if not os.path.isfile(filename):# or (hash_online != has_download):
                    with open(filename, 'wb') as fd:
                        r = requests.get(album['artworkUrl100']  , stream=True)
                        for chunk in r.iter_content(chunk_size=128):
                            fd.write(chunk)

                mpris_player_properties["Metadata"]["mpris:artUrl"] = "file://" + filename
                break
            if albuminfo['resultCount'] == 0:
                mpris_player_properties["Metadata"]["mpris:artUrl"] = "file:///home/10f7c7/Images/funntcat.jpg"

    


def main():
    DBusGMainLoop(set_as_default=True)
    loop = GLib.MainLoop()
    global bus
    bus = dbus.SessionBus()

    global amsbus
    amsbus = bus.get_object("dev.anjay.ams", '/dev/anjay/ams')

    global mpris_dbus
    mpris_dbus = Command()

    initGet()

    om = dbus.Interface(amsbus, DBUS_PROP_IFACE)
    om.connect_to_signal('PropertiesChanged', changed)

    loop.run()

if __name__ == '__main__':
    main()