#include <simpleble/SimpleBLE.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>
#include "ble-const.hpp"
#include "BLE.hpp"
#include "AMSServer.hpp"
#include "ANCSServer.hpp"



struct AMS_ENTITY_ATTR
{
    uint8_t EntityID;
    uint8_t AttributeID;
    uint8_t EntityUpdateFlags;
    std::string Value;
} ams_entity;

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



void ams_notify_callback(SimpleBLE::ByteArray payload) {
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

    ams_entity.EntityID = i[0];
    ams_entity.AttributeID = i[1];
    ams_entity.EntityUpdateFlags = i[2];
    ams_entity.Value = payload.substr(3, payload.size()-3);

    // std::cout << "EntityID: " << (int)ams_entity.EntityID << std::endl;
    // std::cout << "AttributeID: " << (int)ams_entity.AttributeID << std::endl;
    // std::cout << "EntityUpdateFlags: " << (int)ams_entity.EntityUpdateFlags << std::endl;
    // std::cout << "Value: " << ams_entity.Value << std::endl << std::endl << std::endl;

    int data = (std::bitset<8>(ams_entity.EntityID).to_ulong() << 4) | std::bitset<8>(ams_entity.AttributeID).to_ulong();

    // std::cout << std::hex << data << std::endl;

    g_pAMSServer->transferData(data, ams_entity.Value);
}

void ancs_notify_callback(SimpleBLE::ByteArray payload)  {
    int i[payload.size()];
    for (int byte = 0; byte < payload.size(); byte++) {
        i[byte] = (int)payload[byte];
        std::cout << i[byte] << " ";
    }
    std::cout << std::endl;

    ANCS_NOTIF_SRC_ATTR ancs_notif_src;

    ancs_notif_src.EventID = i[0];
    ancs_notif_src.EventFlags = i[1];
    ancs_notif_src.CategoryID = i[2]; //unused here
    ancs_notif_src.CategoryCount = i[3]; //unused here
    ancs_notif_src.NotificationUID = { i[4], i[5], i[6], i[7] };
    ancs_notif_src.NotificationUIDDec = (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];

    std::bitset<32> x(ancs_notif_src.NotificationUIDDec);

    std::cout << "EventID: " << ancs_notif_src.NotificationUID[0] << ", " << ancs_notif_src.NotificationUID[1] << ", " << ancs_notif_src.NotificationUID[2] << ", " << ancs_notif_src.NotificationUID[3] << std::endl;
    std::cout << "EventID: " << x << std::endl;

    g_pANCSServer->transferData(std::move(ancs_notif_src));


}

void ancs_data_callback(SimpleBLE::ByteArray payload) {
    int i[payload.size()];
    for (int byte = 0; byte < payload.size(); byte++) {
        i[byte] = (int)payload[byte];
        std::cout << i[byte] << " ";
    }
    std::cout << std::endl;
}


void start_ams(SimpleBLE::Peripheral phone) {
    std::this_thread::sleep_for(std::chrono::microseconds(400));

    try{
        phone.notify(AMS_UUID, AMS_ENTITY_UPDATE_UUID, ams_notify_callback);
        phone.write_request(AMS_UUID, AMS_ENTITY_UPDATE_UUID, { 0x02, 0x00, 0x01, 0x02, 0x03 });
        phone.write_request(AMS_UUID, AMS_ENTITY_UPDATE_UUID, { 0x00, 0x00, 0x01, 0x02 });
        phone.write_request(AMS_UUID, AMS_ENTITY_UPDATE_UUID, { 0x01, 0x00, 0x01, 0x02, 0x03 });
    }
    catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}

void start_ancs(SimpleBLE::Peripheral phone) {
    std::this_thread::sleep_for(std::chrono::microseconds(400));

    try{
        phone.notify(ANCS_UUID, ANCS_NOTIF_SRC_UUID, ancs_notify_callback);
        phone.notify(ANCS_UUID, ANCS_DATA_SRC_UUID, ancs_data_callback);
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

    auto phone = peripherals[0];

    bool connect_was_successful = phone.connect();

    while (!connect_was_successful) {
        std::cout << "Failed to connect to " << phone.identifier().value_or("UNKNOWN") << " ["
            << phone.address().value_or("UNKNOWN") << "]" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Retrying..." << std::endl;
        connect_was_successful = phone.connect();
    }

    if (connect_was_successful) {
        std::cout << "Starting -AMS-/ANCS" << std::endl;
        // start_ams(phone);
        start_ancs(phone);
    }

    connection = phone;


    while (ble_async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        while (!connection.is_connected()) {
            std::cout << "Failed to connect to " << phone.identifier().value_or("UNKNOWN") << " ["
                << phone.address().value_or("UNKNOWN") << "]" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Retrying..." << std::endl;
            connect_was_successful = phone.connect();
            if (connect_was_successful) {
                std::cout << "Starting -AMS-/ANCS" << std::endl;
                // start_ams(phone);
                start_ancs(phone);
            }
        }
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
    connection.write_request(AMS_UUID, AMS_REMOTE_COMMAND_UUID, { static_cast<char>(command) });
    return command;
}

int CBLE::sendNotification(ANCS_NOTIF_SRC_ATTR ancs_notif_src, std::vector<uint8_t> actions) {
    
    SimpleBLE::ByteArray data = { 0x00, static_cast<char>(ancs_notif_src.NotificationUID[0]), static_cast<char>(ancs_notif_src.NotificationUID[1]), static_cast<char>(ancs_notif_src.NotificationUID[2]), static_cast<char>(ancs_notif_src.NotificationUID[3]), (0x00), (0x01), static_cast<int8_t>(0xff), static_cast<int8_t>(0xff), (0x02), static_cast<int8_t>(0xff), static_cast<int8_t>(0xff), (0x03), static_cast<int8_t>(0xff), static_cast<int8_t>(0xff), (0x04), (0x05) };

    // for (auto i : data) {
    //     // str += static_cast<char>(i);
    //     std::cout << std::bitset<8>(static_cast<int>(i)).to_string() << " "; //to binary
    //     std::cout << std::hex << static_cast<int>(i) << " | ";
    // }

    // std::cout << std::endl;

    for (auto i : actions)  {
        data.push_back(static_cast<char>(i));
    }
    
    connection.write_request(ANCS_UUID, ANCS_CTRL_PT_UUID, data);

    return 1;
}