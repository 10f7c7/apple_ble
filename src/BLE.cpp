#include <simpleble/SimpleBLE.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>

#include "ams.hpp"
#include "BLE.hpp"
#include "Server.hpp"



struct ENTITY_ATTR
{
    uint8_t EntityID;
    uint8_t AttributeID;
    uint8_t EntityUpdateFlags;
    std::string Value;
} entity;


std::atomic_bool ble_async_thread_active = false;
void ble_async_thread_function() {
    while (ble_async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void ble_millisecond_delay(int ms) {
    for (int i = 0; i < ms; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}



void notify_callback(SimpleBLE::ByteArray payload) {
    // std::cout << "Notification received: " << payload << std::endl;
   
    // for (auto byte : payload) {
    //     std::cout << "(" <<  byte << ": ";
    //     std::cout << "" <<  (int)byte << ") ";
    //     std::cout << std::hex << (int)byte << " ";
    // }
    int i[payload.size()];
    for (int byte = 0; byte < payload.size(); byte++) {
        i[byte] = (int)payload[byte];
    }

    // std::cout << payload.substr(3, payload.size()-3) << std::endl;
    // std::cout << std::endl;
    // int value;
    // std::memcpy(&value, static_cast<const void*>(payload.data()), sizeof(int));
    // std::cout << value << std::endl;

    entity.EntityID = i[0];
    entity.AttributeID = i[1];
    entity.EntityUpdateFlags = i[2];
    entity.Value = payload.substr(3, payload.size()-3);

    // std::cout << "EntityID: " << (int)entity.EntityID << std::endl;
    // std::cout << "AttributeID: " << (int)entity.AttributeID << std::endl;
    // std::cout << "EntityUpdateFlags: " << (int)entity.EntityUpdateFlags << std::endl;
    // std::cout << "Value: " << entity.Value << std::endl << std::endl << std::endl;

    int data = (std::bitset<8>(entity.EntityID).to_ulong() << 4) | std::bitset<8>(entity.AttributeID).to_ulong();

    // std::cout << std::hex << data << std::endl;

    g_pServer->transferData(data, entity.Value);


}





void start_ams(SimpleBLE::Peripheral phone) {
    std::this_thread::sleep_for(std::chrono::microseconds(400));

    try{
    phone.notify(AMS_UUID, ENTITY_UPDATE_UUID, notify_callback);
    phone.write_request(AMS_UUID, ENTITY_UPDATE_UUID, { 0x02, 0x00, 0x01, 0x02, 0x03 });
    phone.write_request(AMS_UUID, ENTITY_UPDATE_UUID, { 0x00, 0x00, 0x01, 0x02 });
    phone.write_request(AMS_UUID, ENTITY_UPDATE_UUID, { 0x01, 0x00, 0x01, 0x02, 0x03 });
    }
    catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}


void CBLE::init() {
    // std::thread* ble_async_thread = new std::thread(ble_async_thread_function);

    auto adapter_list = SimpleBLE::Safe::Adapter::get_adapters();

    if (!adapter_list.has_value()) {
        std::cout << "Failed to list adapters" << std::endl;
        return;
    }

    if (adapter_list->empty()) {
        std::cout << "No adapter was found." << std::endl;
        return;
    }
    // Use the first adapter
    SimpleBLE::Safe::Adapter& adapter = adapter_list->at(0);

    bool ble_scan_on = false;

    // Do something with the adapter
    std::cout << "Adapter identifier: " << adapter.identifier().value() << std::endl;
    std::cout << "Adapter address: " << adapter.address().value() << std::endl;

    std::vector<SimpleBLE::Safe::Peripheral> peripherals;

    adapter.set_callback_on_scan_found([&](SimpleBLE::Safe::Peripheral peripheral) {
        if (peripheral.address().value() == PHONE_ADDRESS) {
            std::cout << "Found device: " << peripheral.identifier().value_or("UNKNOWN") << " [" << peripheral.address().value_or("UNKNOWN") << "]" << std::endl;
            peripherals.push_back(peripheral);
            adapter.scan_stop();
        }
    });

    adapter.set_callback_on_scan_start([&]() { 
        std::cout << "Scan started." << std::endl;
        ble_scan_on = true;
    });
    adapter.set_callback_on_scan_stop([&]() { 
        std::cout << "Scan stopped." << std::endl; 
        ble_scan_on = false;    
    });

    adapter.scan_start();
    ble_async_thread_active = true;

    while (ble_scan_on) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        
    }
    // if (!phone.is_connected()) {

    auto phone = peripherals[0];

    bool connect_was_successful = phone.connect();
    // }
    if (connect_was_successful) {
        std::cout << "Starting AMS" << std::endl;
        start_ams(phone);
    }

    connection = phone;


    while (ble_async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    phone.disconnect();

    // return 0;
}




int CBLE::disconnect() {
    // Stop the I/O event loop on the bus connection.
    // This will cause the connection to disconnect from the bus and then return.
    // The connection will be destroyed when the last reference to it goes out of scope.
    connection.disconnect();
    return 0;
}

void CBLE::disconnectThread() {
    ble_async_thread_active = false;
    // while (!ble_async_thread->joinable()) {
    //     ble_millisecond_delay(10);
    // }
    // ble_async_thread->join();
    // delete ble_async_thread;
    // connection.disconnect();
}

std::string CBLE::transferData(std::string data) {
    std::cout << "transferData: " << data << std::endl;
    return data;
}

int CBLE::sendCommand(int command) {
    connection.write_request(AMS_UUID, REMOTE_COMMAND_UUID, {static_cast<char>(command)});
    return command;
}