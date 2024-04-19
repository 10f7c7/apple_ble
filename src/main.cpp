#include <thread>
#include <atomic>
#include <iostream>

#include "Server.hpp"
#include "BLE.hpp"




std::atomic_bool async_thread_active = true;
void async_thread_function() {
    while (async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void millisecond_delay(int ms) {
    for (int i = 0; i < ms; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


int main()  {
    // std::thread* async_thread = new std::thread(async_thread_function);

    g_pServer = std::make_unique<CServer>();
    std::thread* server_async_thread = new std::thread(&CServer::init, g_pServer.get());
    // g_pServer->init();
    std::cout << "started" << std::endl;
    g_pBLE = std::make_unique<CBLE>();
    std::thread* ble_async_thread = new std::thread(&CBLE::init, g_pBLE.get());
    // g_pBLE->init();


    while (!server_async_thread->joinable()) {
        millisecond_delay(10);
    }
    server_async_thread->join();
    delete server_async_thread;
    while (!ble_async_thread->joinable()) {
        millisecond_delay(10);
    }
    ble_async_thread->join();
    delete ble_async_thread;

    std::cout << "done" << std::endl;
    // g_pBLE->disconnect();


    return 0;
}