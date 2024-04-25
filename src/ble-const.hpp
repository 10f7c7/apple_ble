#include <string>
#include <sdbus-c++/sdbus-c++.h>
#pragma once
const sdbus::ObjectPath AMS_MPRIS_OPATH = sdbus::ObjectPath{"/org/mpris/MediaPlayer2"};
const sdbus::InterfaceName AMS_MPRIS_IFACE = sdbus::InterfaceName{ "org.mpris.MediaPlayer2" };
const sdbus::InterfaceName AMS_MPRIS_PLAYER_IFACE = sdbus::InterfaceName{ "org.mpris.MediaPlayer2.Player" };
const sdbus::BusName AMS_MPRIS_BUS_NAME = sdbus::BusName{ "org.mpris.MediaPlayer2.ams" };

const std::string PHONE_ADDRESS = "B4:56:E3:B8:76:DA";
const std::string AMS_UUID = "89d3502b-0f36-433a-8ef4-c502ad55f8dc";
const std::string REMOTE_COMMAND_UUID = "9b3c81d8-57b1-4a8a-b8df-0e56f7ca51c2";
const std::string ENTITY_UPDATE_UUID = "2f7cabce-808d-411f-9a0c-bb92ba96c102";
const std::string ENTITY_ATTR_UUID = "c6b2f38c-23ab-46d8-a6ab-a3a870bbd5d7";