#pragma once

#include <vector>
#include <memory>
#include <tuple>
#include <variant>
#include <string>
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>
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

    std::map<uint32_t, uint8_t> notification_index;
private:
    std::thread* ancs_server_async_thread;
};

inline std::unique_ptr<CANCSServer> g_pANCSServer;