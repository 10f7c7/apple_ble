#include <string>
#include <vector>
#include <sdbus-c++/sdbus-c++.h>
#pragma once
const sdbus::ObjectPath AMS_MPRIS_OPATH = sdbus::ObjectPath{"/org/mpris/MediaPlayer2"};
const sdbus::InterfaceName AMS_MPRIS_IFACE = sdbus::InterfaceName{ "org.mpris.MediaPlayer2" };
const sdbus::InterfaceName AMS_MPRIS_PLAYER_IFACE = sdbus::InterfaceName{ "org.mpris.MediaPlayer2.Player" };
const sdbus::BusName AMS_MPRIS_BUS_NAME = sdbus::BusName{ "org.mpris.MediaPlayer2.ams" };

const sdbus::ObjectPath ANCS_NOTIFICATIONS_OPATH = sdbus::ObjectPath{ "/org/freedesktop/Notifications" };
const sdbus::ServiceName ANCS_NOTIFICATIONS_SNAME = sdbus::ServiceName{ "org.freedesktop.Notifications" };
const sdbus::InterfaceName ANCS_NOTIFICATIONS_IFACE = sdbus::InterfaceName{ "org.freedesktop.Notifications" };
const sdbus::SignalName ANCS_NOTIFICATIONS_SIGNAL = sdbus::SignalName{ "ActionInvoked" };
const sdbus::MethodName ANCS_NOTIFICATIONS_NOTIFY_METHOD = sdbus::MethodName{ "Notify" };
const sdbus::MethodName ANCS_NOTIFICATIONS_CLOSE_METHOD = sdbus::MethodName{ "CloseNotification" };


const std::string PHONE_ADDRESS = "B4:56:E3:B8:76:DA";

const std::string AMS_UUID = "89d3502b-0f36-433a-8ef4-c502ad55f8dc";
const std::string AMS_REMOTE_COMMAND_UUID = "9b3c81d8-57b1-4a8a-b8df-0e56f7ca51c2";
const std::string AMS_ENTITY_UPDATE_UUID = "2f7cabce-808d-411f-9a0c-bb92ba96c102";
const std::string AMS_ENTITY_ATTR_UUID = "c6b2f38c-23ab-46d8-a6ab-a3a870bbd5d7";

const std::string ANCS_UUID = "7905f431-b5ce-4e99-a40f-4b1e122d00d0";
const std::string ANCS_NOTIF_SRC_UUID = "9fbf120d-6301-42d9-8c58-25e699a21dbd";
const std::string ANCS_CTRL_PT_UUID = "69d1d8f3-45e1-49a8-9821-9bbdfdaad9d9";
const std::string ANCS_DATA_SRC_UUID = "22eac6e9-24d6-4bb5-be44-b36ace7c7bfb";

struct ANCS_NOTIF_SRC_ATTR
{
    uint8_t EventID;
    uint8_t EventFlags;
    uint8_t CategoryID;
    uint8_t CategoryCount;
    std::vector<int> NotificationUID;
    uint32_t NotificationUIDDec;
};