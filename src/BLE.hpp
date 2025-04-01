#pragma once

#include "ble-const.hpp"
#include <memory>
#include <simpleble/SimpleBLE.h>
#include <vector>

class CBLE {
  public:
    void init();
    int disconnect();
    std::string transferData(std::string);
    int sendCommand(int);
    int sendNotification(ANCS_NOTIF_SRC_ATTR, std::vector<uint8_t>);
    int sendApplicationName(std::string);
    int sendNotificationAction(std::vector<int>, int8_t);
    void disconnectThread();

    SimpleBLE::Peripheral connection;

  private:
    std::thread *ble_async_thread;
};

inline std::unique_ptr<CBLE> g_pBLE;
