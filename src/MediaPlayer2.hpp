#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include "server-glue.hpp"
#include "BLE.hpp"
#include "Server.hpp"

class MediaPlayer2 : public sdbus::AdaptorInterfaces<org::mpris::MediaPlayer2_adaptor, sdbus::Properties_adaptor /*, more adaptor classes if there are more interfaces*/>
{
public:
    MediaPlayer2(sdbus::IConnection& connection, std::string objectPath)
        : AdaptorInterfaces(connection, std::move(objectPath)) {
        registerAdaptor();
    }

    ~MediaPlayer2() {
        unregisterAdaptor();
    }

protected:

    void Raise() override {
        std::cout << "raise" << std::endl;
    }

    void Quit() override {
        std::cout << "quit" << std::endl;
        g_pBLE->disconnectThread();
        g_pServer->disconnectThread();
    }

    bool CanQuit() override {
        return m_CanQuit;
    }

    bool CanRaise() override {
        return m_CanRaise;
    }

    bool HasTrackList() override {
        return m_HasTrackList;
    }

    std::string Identity() override {
        return m_Identity;
    }

    std::vector<std::string> SupportedUriSchemes() override {
        return m_SupportedUriSchemes;
    }

    std::vector<std::string> SupportedMimeTypes() override {
        return m_SupportedMimeTypes;
    }
    
private:

    bool m_CanQuit = true;
    bool m_CanRaise = true;
    bool m_HasTrackList = false;
    std::string m_Identity = "AMS";
    std::vector<std::string> m_SupportedUriSchemes = { "ams" };
    std::vector<std::string> m_SupportedMimeTypes = { "application/ams" };
public:
    
};

inline std::unique_ptr<MediaPlayer2> g_pMediaPlayer2;