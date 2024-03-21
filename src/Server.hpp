#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <sdbus-c++/sdbus-c++.h>
#include "Player.hpp"


class CServer {
    public:
        void init();
        std::string transferData(int, std::string);
        void disconnectThread();
        std::unique_ptr<sdbus::IConnection> connection;


    private:
        std::thread* server_async_thread;
        std::unique_ptr<Player> player;
};

inline std::unique_ptr<CServer> g_pServer;