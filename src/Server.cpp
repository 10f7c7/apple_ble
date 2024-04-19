#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <tuple>
#include <memory>
#include <sstream>
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

    connection->enterEventLoopAsync();
    // Player player(*connection, AMS_MPRIS_OPATH);
    g_pPlayer = std::make_unique<Player>(*connection, AMS_MPRIS_OPATH);
    // MediaPlayer2 mediaplayer2(*connection, AMS_MPRIS_OPATH);
    g_pMediaPlayer2 = std::make_unique<MediaPlayer2>(*connection, AMS_MPRIS_OPATH);

    // Run the I/O event loop on the bus connection.
    // while (!server_async_thread->joinable()) {
    //     server_millisecond_delay(10);
    // }
    // server_async_thread->join();
    // delete server_async_thread;
    while (server_async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // connection->releaseName(AMS_MPRIS_BUS_NAME);
    // try{
    // connection->leaveEventLoop();
    // } catch (const std::exception& e) {
    //     std::cerr << e.what() << std::endl;
    // }
    return;

    // g_pBLE->transferData("helloworld");
}

std::string replace_all(std::string s, std::string const& toReplace, std::string const& replaceWith) {
    std::string buf;
    std::size_t pos = 0;
    std::size_t prevPos;

    // Reserves rough estimate of final size of string.
    buf.reserve(s.size());

    while (true) {
        prevPos = pos;
        pos = s.find(toReplace, pos);
        if (pos == std::string::npos)
            break;
        buf.append(s, prevPos, pos - prevPos);
        buf += replaceWith;
        pos += toReplace.size();
    }

    buf.append(s, prevPos, s.size() - prevPos);
    s.swap(buf);
    return s;
}

std::string CServer::transferData(int id, std::string data) {
    std::cout << "transferData: " << data << std::endl;
    // g_pBLE->transferData("recived");

    std::string key;

    if ((id >> 4) == 0b0000) {

                std::cout << "EntityIDPlayer" << std::endl;
                if (id == 0x00) {
                    
                } else if (id == 0x01) {
                    std::tuple<int, float, float> dataTuple;
                    std::stringstream dataStr(data);
                    std::string PlaybackState, PlaybackRate, ElapsedTime;

                    std::getline(dataStr, PlaybackState, ',');
                    std::getline(dataStr, PlaybackRate, ',');
                    std::getline(dataStr, ElapsedTime, ',');

                    int PlaybackStateInt = std::move(stoi(PlaybackState));
                    float PlaybackRateFloat = std::move(stof(PlaybackRate));
                    int64_t ElapsedTimeFloat = std::move(stof(ElapsedTime) * 1000000);

                    std::cout << "PlaybackState: " << stoi(PlaybackState) << std::endl;
                                        std::string playbackStatus;
                    if (PlaybackStateInt == 0) {
                        playbackStatus = "Paused";
                    } else if (PlaybackStateInt == 1) {
                        playbackStatus = "Playing";
                    } else if (PlaybackStateInt == 2) {
                        playbackStatus = "Rewinding";
                    } else if (PlaybackStateInt == 3) {
                        playbackStatus = "FastForwarding";
                    }
                    std::cout << "playbackStatus: " << playbackStatus << std::endl;
                    g_pPlayer->updatePlaybackStatus(playbackStatus);
                    g_pPlayer->updatePlaybackRate(PlaybackRateFloat);
                    g_pPlayer->updateElapsedTime(ElapsedTimeFloat);

                } else if (id == 0x02) {

                }
    }

    if ((id >> 4) & 0b0010) {
        std::cout << "EntityIDTrack" << std::endl;
        if (id == 0x20) {
            key = "xesam:artist";
        } else if (id == 0x21) {
            key = "xesam:album";
            g_pPlayer->updateMetadata("mpris:artUrl", "file:///home/10f7c7/Projects/apple_ble/album_icons/" + replace_all(data, " ", "+"));
        } else if (id == 0x22) {
            key = "xesam:title";
        } else if (id == 0x23) {
            key = "mpris:length";
            std::cout << data  << std::endl;
            data = std::to_string(std::stoi(data) * 1000000);
            std::cout << data  << std::endl;
        }
        g_pPlayer->updateMetadata(key, data);
    }

    std::cout << std::endl;

    return data;
}

void CServer::disconnectThread() {
    connection->releaseName(AMS_MPRIS_BUS_NAME);
    std::cout << "Server disconnected" << std::endl;
    connection->leaveEventLoop();
    server_async_thread_active = false;
    // while (!server_async_thread->joinable()) {
    //     server_millisecond_delay(10);
    // }
    // server_async_thread->join();
    // delete server_async_thread;


}


