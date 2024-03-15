#include <simpleble/SimpleBLE.h>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cstring>

#include "ams.h"



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





// class ENTITY_ATTR :
//     def __init__(self, data) :
//     self.EntityID = int(data[0])
//     self.AttributeID = int(data[1])
//     self.EntityUpdateFlags = int(data[2])
//     self.Value = bytes(data[3:]).decode()#''.join([str(v) for v in data[3:]])

struct ENTITY_ATTR
{
    uint8_t EntityID;
    uint8_t AttributeID;
    uint8_t EntityUpdateFlags;
    std::string Value;
} entity;




void notify_callback(SimpleBLE::ByteArray payload) {
    std::cout << "Notification received: " << payload << std::endl;
   
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

    std::cout << "EntityID: " << (int)entity.EntityID << std::endl;
    std::cout << "AttributeID: " << (int)entity.AttributeID << std::endl;
    std::cout << "EntityUpdateFlags: " << (int)entity.EntityUpdateFlags << std::endl;
    std::cout << "Value: " << entity.Value << std::endl << std::endl << std::endl;

}





void start_ams(SimpleBLE::Peripheral phone) {
    phone.notify(AMS_UUID, ENTITY_UPDATE_UUID, notify_callback);
    phone.write_request(AMS_UUID, ENTITY_UPDATE_UUID, { 0x02, 0x00, 0x01, 0x02, 0x03 });
    phone.write_request(AMS_UUID, ENTITY_UPDATE_UUID, { 0x00, 0x00, 0x01, 0x02 });
    phone.write_request(AMS_UUID, ENTITY_UPDATE_UUID, { 0x01, 0x00, 0x01, 0x02, 0x03 });
}


int main() {
    std::thread* async_thread = new std::thread(async_thread_function);

    if (!SimpleBLE::Adapter::bluetooth_enabled()) {
        std::cout << "Bluetooth is not enabled" << std::endl;
        return 1;
    }

    auto adapters = SimpleBLE::Adapter::get_adapters();
    if (adapters.empty()) {
        std::cout << "No Bluetooth adapters found" << std::endl;
        return 1;
    }

    // Use the first adapter
    auto adapter = adapters[0];

    // Do something with the adapter
    std::cout << "Adapter identifier: " << adapter.identifier() << std::endl;
    std::cout << "Adapter address: " << adapter.address() << std::endl;

    SimpleBLE::Peripheral phone;

    adapter.set_callback_on_scan_found([&adapter, &phone](SimpleBLE::Peripheral peripheral) {
        // std::cout << "Peripheral found: " << peripheral.address() << std::endl;
        if (peripheral.address() == PHONE_ADDRESS) {
            std::cout << "found phone" << std::endl;
            adapter.scan_stop();
            peripheral.connect();
            std::cout << "connected" << std::endl;
            start_ams(peripheral);
        }
    });

    // Start scanning for peripherals
    adapter.scan_start();

    // Wait for 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop scanning for peripherals
    if (adapter.scan_is_active()) {
        // std::cout << "Stopping scan" << std::endl;
        adapter.scan_stop();
    }

    async_thread_active = true;
    while (!async_thread->joinable()) {
        millisecond_delay(10);
    }
    async_thread->join();
    delete async_thread;


    phone.disconnect();
    
    return 0;
}