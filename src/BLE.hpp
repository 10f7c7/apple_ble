#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <simpleble/SimpleBLE.h>

class CBLE {
public:
    void init();
    int disconnect();
    std::string transferData(std::string);
    int sendCommand(int);
    void disconnectThread();

    SimpleBLE::Peripheral connection;
private:
    std::thread* ble_async_thread;

};

inline std::unique_ptr<CBLE> g_pBLE;