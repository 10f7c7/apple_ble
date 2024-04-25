#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <variant>
#include <string>
#include <iostream>
#include "ams-server-glue.hpp"
#include "BLE.hpp"

using sdbus::ObjectPath;

class Player : public sdbus::AdaptorInterfaces<org::mpris::MediaPlayer2::Player_adaptor, sdbus::Properties_adaptor /*, more adaptor classes if there are more interfaces*/>
{
public:
    Player(sdbus::IConnection& connection, sdbus::ObjectPath objectPath)
        : AdaptorInterfaces(connection, std::move(objectPath)) {
        registerAdaptor();
    }

    ~Player() {
        unregisterAdaptor();
    }

protected:
    void Next() override {
        std::cout << "next" << std::endl;
        g_pBLE->sendCommand(0x03);
    }

    void Previous() override {
        std::cout << "previous" << std::endl;
        g_pBLE->sendCommand(0x04);
    }

    void Pause() override {
        std::cout << "pause" << std::endl;
        m_PlaybackStatus = "Paused";
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{Player_adaptor::INTERFACE_NAME}, {sdbus::PropertyName{"PlaybackStatus"}});
    }

    void PlayPause() override {
        std::cout << "playpause" << std::endl;
        if (m_PlaybackStatus == "Paused")  {
            m_PlaybackStatus = "Playing";
        } else if (m_PlaybackStatus == "Playing")  {
            m_PlaybackStatus = "Paused";
        }

        // g_pBLE->transferData("playpause");
        g_pBLE->sendCommand(0x02);

        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"PlaybackStatus"} });
    }

    void Stop() override {
        std::cout << "stop" << std::endl;
        m_PlaybackStatus = "Stopped";
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"PlaybackStatus"} });
    }

    void Play() override {
        std::cout << "play" << std::endl;
        m_PlaybackStatus = "Playing";
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"PlaybackStatus"} });
    }

    void Seek(const int64_t& Offset) override {
        std::cout << "seek" << std::endl;
    }

    void SetPosition(const sdbus::ObjectPath& TrackId, const int64_t& Position) override {
        std::cout << "setposition" << std::endl;
        m_Position = Position;
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"Position"} });
    }

    void OpenUri(const std::string& Uri) override {
        std::cout << "openuri" << std::endl;
    }

    std::string PlaybackStatus() override { 
        std::cout << "playbackstatus" << std::endl;
        return m_PlaybackStatus;
    }

    double Rate() override {
        std::cout << "rate" << std::endl;
        return m_Rate;
    }

    void Rate(const double& value) override {
        std::cout << "rate" << std::endl;
        m_Rate = value;
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"Rate"} });
    }

    std::map<std::string, std::variant<int64_t, std::string, std::vector<std::string>>> Metadata() override {
        std::cout << "metadata" << std::endl;
        return m_Metadata;
    }

    double Volume() override {
        std::cout << "volume" << std::endl;
        return m_Volume;
    }

    void Volume(const double& value) override {
        std::cout << "volume" << std::endl;
        m_Volume = value;
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"Volume"} });
    }

    int64_t Position() override {
        std::cout << "position" << std::endl;
        return m_Position;
    }

    double MinimumRate() override {
        std::cout << "minimumrate" << std::endl;
        return m_MinimumRate;
    }

    double MaximumRate() override {
        std::cout << "maximumrate" << std::endl;
        return m_MaximumRate;
    }

    bool CanGoNext() override {
        std::cout << "cangonext" << std::endl;
        return m_CanGoNext;
    }

    bool CanGoPrevious() override {
        std::cout << "cangoprevious" << std::endl;
        return m_CanGoPrevious;
    }

    bool CanPlay() override {
        std::cout << "canplay" << std::endl;
        return m_CanPlay;
    }

    bool CanPause() override {
        std::cout << "canpause" << std::endl;
        return m_CanPause;
    }

    bool CanSeek() override {
        std::cout << "canseek" << std::endl;
        return m_CanSeek;
    }

    bool CanControl() override {
        std::cout << "cancontrol" << std::endl;
        return m_CanControl;
    }

private:
    std::string m_PlaybackStatus = "Paused";
    double m_Rate = 0.0;
    std::map<std::string, std::variant<int64_t, std::string, std::vector<std::string>>> m_Metadata;
    double m_Volume = 0.0;
    int64_t m_Position = 0;
    double m_MinimumRate = 0.0;
    double m_MaximumRate = 0.0;
    bool m_CanGoNext = true;
    bool m_CanGoPrevious = true;
    bool m_CanPlay = true;
    bool m_CanPause = true;
    bool m_CanSeek = false;
    bool m_CanControl = true;

public:
    void updateMetadata(const std::string key, const std::string value) {
        if (m_Metadata.find(key) != m_Metadata.end()) {
            m_Metadata.erase(key);
        }
        if ((key == "mpris:length")) {
            m_Metadata.insert({ key, int64_t(std::stoi(value))});
        }
         else if (key == "xesam:artist") {
            m_Metadata.insert({ key, (std::vector<std::string>){value}});
        } 
        else {
            m_Metadata.insert({ key, value });
        }
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"Metadata"} });
    }

    std::string getMetadata(const std::string key) {
        if (std::holds_alternative<std::string>(m_Metadata[key])) {
            return std::get<std::string>(m_Metadata[key]);
        }
        else if (std::holds_alternative<std::vector<std::string>>(m_Metadata[key])) {
            return std::get<std::vector<std::string>>(m_Metadata[key])[0];
        }
        else if (std::holds_alternative<int64_t>(m_Metadata[key])) {
            return std::to_string(std::get<int64_t>(m_Metadata[key]));
        }
        else {
            return "";
        }
    }

    void updatePlaybackStatus(const std::string status) {
        m_PlaybackStatus = status;
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"PlaybackStatus"} });
    }

    void updatePlaybackRate(const double rate) {
        m_Rate = rate;
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"Rate"} });
    }

    void updateElapsedTime(const int64_t time) {
        m_Position = time;
        Properties_adaptor::emitPropertiesChangedSignal(sdbus::InterfaceName{ Player_adaptor::INTERFACE_NAME }, { sdbus::PropertyName{"Position"} });
    }
};

inline std::unique_ptr<Player> g_pPlayer;