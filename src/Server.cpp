#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "Player.hpp"
#include "MediaPlayer2.hpp"
#include "Server.hpp"

#include "BLE.hpp"


std::atomic_bool server_async_thread_active = true;
void server_async_thread_function() {
    while (server_async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void server_millisecond_delay(int ms) {
    for (int i = 0; i < ms; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


void CServer::init() {
    // Create D-Bus connection to the system bus and requests name on it.
    // const char* serviceName = "org.sdbuscpp.concatenator";
    std::thread* server_async_thread = new std::thread(server_async_thread_function);

    connection = sdbus::createSessionBusConnection(AMS_MPRIS_BUS_NAME);

    // Create concatenator D-Bus object.
    // const char* objectPath = "/org/sdbuscpp/concatenator";
    // auto mpris_opath = sdbus::createObject(*connection, AMS_MPRIS_OPATH);

    Player player(*connection, AMS_MPRIS_OPATH);
    MediaPlayer2 mediaplayer2(*connection, AMS_MPRIS_OPATH);

    // Run the I/O event loop on the bus connection.
    connection->enterEventLoopAsync();
    // while (!server_async_thread->joinable()) {
    //     server_millisecond_delay(10);
    // }
    // server_async_thread->join();
    // delete server_async_thread;
    while (server_async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    connection->releaseName(AMS_MPRIS_BUS_NAME);
    connection->leaveEventLoop();



    // g_pBLE->transferData("helloworld");
}

std::string CServer::transferData(std::string data) {
    std::cout << "transferData: " << data << std::endl;
    g_pBLE->transferData("recived");

    return data;
}

void CServer::disconnectThread() {
    server_async_thread_active = false;
    // while (!server_async_thread->joinable()) {
    //     server_millisecond_delay(10);
    // }
    // server_async_thread->join();
    // delete server_async_thread;

    // connection->releaseName(AMS_MPRIS_BUS_NAME);
    // connection->leaveEventLoop();

}


