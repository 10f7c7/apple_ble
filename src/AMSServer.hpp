#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <sdbus-c++/sdbus-c++.h>
#include "Player.hpp"


class CAMSServer {
    public:
        void init();
        std::string transferData(int, std::string);
        void disconnectThread();
        std::unique_ptr<sdbus::IConnection> connection;


    private:
        std::thread* amsserver_async_thread;
};

inline std::unique_ptr<CAMSServer> g_pAMSServer;