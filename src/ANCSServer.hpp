#pragma once

#include "../include/tz.h"
#include "Player.hpp"
#include "ble-const.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <thread>
#include <tuple>
#include <variant>
#include <vector>

class CANCSServer {
  public:
    void init();
    std::string transferData(ANCS_NOTIF_SRC_ATTR);
    void disconnectThread();
    enum ANCS_EVENT_FLAGS {
        Silent = 0x00,
        Important = 0x01,
        PreExisting = 0x02,
        PositiveAction = 0x03,
        NegativeAction = 0x04,
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
    enum ANCS_EVENT_ID {
        NotificationAdded = 0x00,
        NotificationModified = 0x01,
        NotificationRemoved = 0x02,
    };
    enum ANCS_COMMAND_ID {
        GetNotificationAttributes = 0x00,
        GetAppAttributes = 0x01,
        PerformNotificationAction = 0x02,
    };
    std::vector<std::variant<std::string, uint32_t>> decodeNotification(std::vector<uint8_t>);
    void processNotification(std::vector<uint8_t>);
    void write_notification(std::vector<std::variant<std::string, uint32_t>>);
    void notification_action(sdbus::Signal);
    std::vector<uint8_t> overflow;
    std::map<uint32_t, uint8_t> notification_index;
    std::map<uint32_t, uint32_t> notification_serverid_index;
    std::vector<std::vector<std::variant<std::string, uint32_t>>> nameless_notification_index;
    std::map<std::string, std::string> application_index;
    std::unique_ptr<sdbus::IProxy> proxy;

    bool isDST = true;

  private:
    std::thread *ancs_server_async_thread;
};

inline std::unique_ptr<CANCSServer> g_pANCSServer;
