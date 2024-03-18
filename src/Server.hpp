#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <sdbus-c++/sdbus-c++.h>


class CServer {
    public:
        void init();
        std::string transferData(std::string data);
        void disconnectThread();
        std::unique_ptr<sdbus::IConnection> connection;


    private:
        std::thread* server_async_thread;

};

inline std::unique_ptr<CServer> g_pServer;