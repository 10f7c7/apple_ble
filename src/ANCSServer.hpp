#pragma once

#include <vector>
#include <memory>
#include <tuple>
#include <variant>
#include <string>
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>
#include <filesystem>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include "Player.hpp"
#include "ble-const.hpp"


class CANCSServer {
public:
    void init();
    std::string transferData(ANCS_NOTIF_SRC_ATTR);
    void disconnectThread();
    enum NOTIF_FLAG {
        SILENT = 0x00,
        IMPORTANT = 0x01,
        PREEXISTING = 0x02,
        POSITIVE_ACTION = 0x03,
        NEGATIVE_ACTION = 0x04,
    };
    enum ANCS_NOTIF_ATTR {
        AppIdentifier = 0x00,
        Title = 0x01,
        Subtitle = 0x02,
        Message = 0x03,
        MessageSize = 0x04,
        Date = 0x05,
        PositiveActionLabel = 0x06,
        NegativeActionLabel = 0x07,
        NotificationUID = 0x08,
    };
    std::vector<std::variant<std::string, uint32_t>> decodeNotification(std::vector<uint8_t>);
    void processNotification(std::vector<uint8_t>);
    void write_notification(std::vector<std::variant<std::string, uint32_t>>);
    std::vector<uint8_t> overflow;
    std::map<uint32_t, uint8_t> notification_index;
    std::map<uint32_t, uint32_t> notification_serverid_index;
    std::vector<std::vector<std::variant<std::string, uint32_t>>> nameless_notification_index;
    std::map<std::string, std::string> application_index;
    std::unique_ptr<sdbus::IProxy> proxy;

private:
    std::thread* ancs_server_async_thread;
};

inline std::unique_ptr<CANCSServer> g_pANCSServer;