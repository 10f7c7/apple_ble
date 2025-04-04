#include "AMSServer.hpp"
#include "ANCSServer.hpp"
#include "BLE.hpp"
#include <atomic>
#include <iostream>
#include <thread>

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

int main() {
    // check if cache dirs exist
    if (!std::filesystem::exists((CACHE_DIR + "/album_icons")) && (!std::filesystem::exists(CACHE_DIR + "/app_icons"))) {
        std::filesystem::create_directories(CACHE_DIR + "/album_icons");
        std::filesystem::create_directories(CACHE_DIR + "/app_icons");
    }

    // std::thread* async_thread = new std::thread(async_thread_function);

    g_pBLE = std::make_unique<CBLE>();
    std::thread *ble_async_thread = new std::thread(&CBLE::init, g_pBLE.get());

    g_pANCSServer = std::make_unique<CANCSServer>();
    std::thread *ancs_server_async_thread =
        new std::thread(&CANCSServer::init, g_pANCSServer.get());

    g_pAMSServer = std::make_unique<CAMSServer>();
    std::thread *ams_server_async_thread =
        new std::thread(&CAMSServer::init, g_pAMSServer.get());

    std::cout << "started" << std::endl;

    // g_pBLE->init();
    // g_pServer->init();

    while (!ble_async_thread->joinable()) {
        millisecond_delay(10);
    }
    ble_async_thread->join();
    delete ble_async_thread;

    while (!ams_server_async_thread->joinable()) {
        millisecond_delay(10);
    }
    ams_server_async_thread->join();
    delete ams_server_async_thread;

    while (!ancs_server_async_thread->joinable()) {
        millisecond_delay(10);
    }
    ancs_server_async_thread->join();
    delete ancs_server_async_thread;

    std::cout << "done" << std::endl;
    // g_pBLE->disconnect();

    return 0;
}
