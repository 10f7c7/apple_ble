#include "ANCSServer.hpp"
#include "../include/rapidjson/document.h"

std::atomic_bool ancs_server_async_thread_active = true;
void ancs_server_async_thread_function() {
    while (ancs_server_async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void ancs_server_millisecond_delay(int ms) {
    for (int i = 0; i < ms; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

std::string CANCSServer::transferData(ANCS_NOTIF_SRC_ATTR ancs_notif_src) {
    std::cout << "transferData" << std::endl;
    std::cout << "EventID: " << ancs_notif_src.NotificationUID << std::endl;
    std::cout << "EventFlags: " << (int)ancs_notif_src.EventFlags << std::endl;
    notification_index.insert({ancs_notif_src.NotificationUID, ancs_notif_src.EventFlags});
    if (ancs_notif_src.EventID == ANCS_EVENT_ID::NotificationRemoved) {
        std::cout << "phone removed : " << notification_serverid_index[ancs_notif_src.NotificationUID] << std::endl;
        auto method = proxy->createMethodCall(ANCS_NOTIFICATIONS_IFACE, ANCS_NOTIFICATIONS_CLOSE_METHOD);
        method << (uint32_t)notification_serverid_index[ancs_notif_src.NotificationUID];
        proxy->callMethod(method);
        notification_serverid_index.erase(ancs_notif_src.NotificationUID);
        notification_index.erase(ancs_notif_src.NotificationUID);
        return "";
    }
    bool preexist = ancs_notif_src.EventFlags & (1 << ANCS_EVENT_FLAGS::PreExisting);

    std::vector<uint8_t> action;

    if (ancs_notif_src.EventFlags & (1 << ANCS_EVENT_FLAGS::PositiveAction)) {
        action.insert(action.end(), 0x06);
    } else if (ancs_notif_src.EventFlags & (1 << ANCS_EVENT_FLAGS::NegativeAction)) {
        action.insert(action.end(), 0x07);
    }

    g_pBLE->sendNotification(ancs_notif_src, action);

    return ""; // Placeholder return value
};

void CANCSServer::write_notification(std::vector<std::variant<std::string, uint32_t>> attr) {
    std::vector<std::string> action;

    bool sendQuiet = false;

    if (notification_index[std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))] & (1 << ANCS_EVENT_FLAGS::PositiveAction)) {
        std::cout << "POSITIVE_ACTION" << std::endl;
        action.insert(action.end(), "10f7c7 0 " + std::to_string((int)std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))));
        action.insert(action.end(), std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::PositiveActionLabel)));
    } else if (notification_index[std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))] & (1 << ANCS_EVENT_FLAGS::NegativeAction)) {
        std::cout << "NEGATIVE_ACTION" << std::endl;
        action.insert(action.end(), "10f7c7 1 " + std::to_string((int)std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))));
        action.insert(action.end(), std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::NegativeActionLabel)));
    }

    if (notification_index[std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))] & (1 << ANCS_EVENT_FLAGS::PreExisting)) {
        sendQuiet = true;
    }

    std::string appli_name;

    if (!application_index[std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::AppIdentifier))].empty()) {
        appli_name = application_index[std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::AppIdentifier))];
        std::cout << "appli_name: " << appli_name << std::endl;
    }

    std::string filename = CACHE_DIR + "/app_icons/" + std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::AppIdentifier)) + ".jpg";
    if (std::filesystem::exists(filename)) {
        std::cout << "icon exists" << std::endl;
    } else {
        std::cout << "icon does not exist" << std::endl;
        httplib::Client cli("https://itunes.apple.com");

        rapidjson::Document iconBody;
        // auto res = cli.Get("/lookup/?bundleId=" + std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::AppIdentifier)));
        cli.Get("/lookup/?bundleId=" + std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::AppIdentifier)), [&](const char *data, size_t data_length) {
            iconBody.Parse(data, data_length);
            return true;
        });
        // nlohmann::json obj = nlohmann::json::parse(res->body);
        // auto bodystream = std::istringstream(res->body);
        // document.ParseStream(bodystream);

        // if (obj.at("resultCount") == 0) {
        if (iconBody["resultCount"].GetInt() == 0) {

            std::cout << "No results" << std::endl;
            // appli_name = "rats";
            std::ofstream fd(filename, std::ios::binary);
            fd.close();
        } else {
            // std::string url = obj.at("results").at(0).at("artworkUrl100").get<std::string>();
            std::string url = iconBody["results"][0]["artworkUrl100"].GetString();
            std::cout << url << std::endl;
            size_t found = url.find_first_of(":");
            std::string url_new = url.substr(found + 3); // url excluding http
            size_t found2 = url_new.find_first_of("/");
            std::string urlBase = url.substr(0, found + 3) + url_new.substr(0, found2); // baseurl
            std::cout << urlBase << std::endl;
            httplib::Client cli2("https://" + url_new.substr(0, found2));
            // auto res2 = cli2.Get(obj.at("results").at(0).at("artworkUrl100").get<std::string>().erase(0, urlBase.size()));
            auto res2 = cli2.Get(url.erase(0, urlBase.size()));
            std::ofstream fd(filename, std::ios::binary);
            fd << res2->body;
            fd.close();
        }
    }
    std::tm tm{};

    tm.tm_year = std::stoi(std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Date)).substr(0, 4)) - 1900;
    tm.tm_mon = std::stoi(std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Date)).substr(4, 2)) - 1;
    tm.tm_mday = std::stoi(std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Date)).substr(6, 2));
    tm.tm_hour = std::stoi(std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Date)).substr(9, 2));
    tm.tm_min = std::stoi(std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Date)).substr(11, 2));
    tm.tm_sec = std::stoi(std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Date)).substr(13, 2));
    tm.tm_isdst = CANCSServer::isDST;

    std::time_t t = std::mktime(&tm);

    auto method = proxy->createMethodCall(ANCS_NOTIFICATIONS_IFACE, ANCS_NOTIFICATIONS_NOTIFY_METHOD);
    std::cout << (notification_serverid_index[std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))]) << std::endl;
    method << appli_name << (uint32_t)(notification_serverid_index[std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))] | 0) << filename << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Title)) << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Message)) << action << std::map<std::string, std::variant<int32_t, bool, std::time_t>>{{"urgency", 1}, {"ancs", true}, {"swaync_send_quiet", sendQuiet}, {"time", t}} << 10000;
    auto reply = proxy->callMethod(method);
    uint32_t result;
    reply >> result;
    // notification_serverid_index.insert({
    // std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID)), result });
    notification_serverid_index[std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))] = result;
    std::cout << "reply: " << std::to_string(result) << " index : " << notification_serverid_index[std::get<uint32_t>(attr.at(ANCS_NOTIF_ATTR::NotificationUID))] << std::endl;
}

void CANCSServer::processNotification(std::vector<uint8_t> data) {
    std::cout << "processNotification" << std::endl;
    std::vector<std::variant<std::string, uint32_t>> attr = decodeNotification(data);
    if (overflow.size() > 0) {
        std::cout << "overflow" << std::endl;
        return;
    }

    if (attr.size() > 3) {
        std::cout << "attr: " << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::AppIdentifier)) << std::endl;
        std::cout << "attr: " << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Title)) << std::endl;
        std::cout << "attr: " << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Subtitle)) << std::endl;
        std::cout << "attr: " << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Message)) << std::endl;
        std::cout << "attr: " << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::MessageSize)) << std::endl;
        std::cout << "attr: " << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::Date)) << std::endl;
        std::cout << "attr: " << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::PositiveActionLabel)) << std::endl;
        std::cout << "attr: " << std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::NegativeActionLabel)) << std::endl;

        if (application_index.find(std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::AppIdentifier))) == application_index.end()) {
            g_pBLE->sendApplicationName(std::get<std::string>(attr.at(ANCS_NOTIF_ATTR::AppIdentifier)));
            // if
            // (application_index.find(std::get<std::string>(attr.at(AppIdentifier)))
            // == application_index.end()) {
            //     application_index.insert({
            //     std::get<std::string>(attr.at(AppIdentifier)), "" });
            // }
            nameless_notification_index.insert(nameless_notification_index.end(), std::move(attr));
            return;
        }

        write_notification(attr);
    }

    if (attr.size() < 4) {
        std::cout << "processApp" << std::endl;

        std::string display_name = "Rats";
        if (!std::get<std::string>(attr.at(0)).empty()) {
            display_name = std::get<std::string>(attr.at(0));
        }
        application_index.insert({std::get<std::string>(attr.at(1)), display_name});
        std::cout << "application_index: " << std::get<std::string>(attr.at(1)) << std::endl;
        std::cout << "display_name: " << std::get<std::string>(attr.at(0)) << std::endl
                  << std::endl;
        std::vector<std::vector<std::variant<std::string, uint32_t>>> temp;
        for (int i = 0; i < nameless_notification_index.size(); i++) {
            if (std::get<std::string>(nameless_notification_index.at(i).at(ANCS_NOTIF_ATTR::AppIdentifier)) == std::get<std::string>(attr.at(1))) {
                write_notification(nameless_notification_index.at(i));
            } else {
                temp.insert(temp.end(), nameless_notification_index.at(i));
            }
        }
        nameless_notification_index = std::move(temp);
    }
}

std::vector<std::variant<std::string, uint32_t>>
CANCSServer::decodeNotification(std::vector<uint8_t> incomedata) {
    std::vector<uint8_t> data = incomedata;
    std::cout << "decodeNotification" << std::endl;
    if (overflow.size() > 0) {
        data.insert(data.begin(), overflow.begin(), overflow.end());
    }
    bool of = false;
    bool eh = false;
    int i = 0;
    int ID = 0;
    // ANCS_NOTIF_ATTR ancs_notif_attr;
    std::vector<std::variant<std::string, uint32_t>> attributes;
    uint8_t CommandID = data[i++];
    std::string AppIdentaifier;
    uint32_t NotificationUID;
    uint8_t flags;
    if (CommandID == ANCS_COMMAND_ID::GetAppAttributes) {
        std::cout << "CommandID: " << (int)CommandID << std::endl;
        while (data[i] != 0) {
            AppIdentaifier += data[i++];
        }
        i++;
    }
    if (CommandID == ANCS_COMMAND_ID::GetNotificationAttributes) {
        NotificationUID = (data[i + 3] << 24) | (data[i + 2] << 16) | (data[i + 1] << 8) | data[i];
        i += 4;
        flags = notification_index[NotificationUID];
    }

    while ((i < data.size()) && !of) {
        if (of)
            std::cout << "of" << i << std::endl;
        if (ID == 0x01 && CommandID == ANCS_COMMAND_ID::GetAppAttributes) {
            eh = true;
            break;
        }
        if (ID == 0x08) {
            eh = true;
            break;
        }
        if (ID > 0x05) {
            if (!(flags & (1 << ANCS_EVENT_FLAGS::PositiveAction))) {
                attributes.insert(attributes.begin() + ID, "");
                ID++;
            }
            if (!(flags & (1 << ANCS_EVENT_FLAGS::NegativeAction))) {
                attributes.insert(attributes.begin() + ID, "");
                ID++;
                break;
            }
        }
        if (data[i] == ID) {
            eh = false;
            int length;
            try {
                length = data[i + 1] | (data[i + 2] << 8);
            } catch (...) {
                of = true;
                std::cout << "Overflowlenerr" << std::endl;
                overflow.insert(incomedata.begin(), overflow.begin(), overflow.end());
                break;
            }
            i += 3;

            if (i + length > data.size()) {
                of = true;
                std::cout << "Overflowtoomuch" << std::endl;
                overflow.insert(incomedata.begin(), overflow.begin(), overflow.end());
                break;
            }
            attributes.insert(
                attributes.begin() + ID,
                std::string(data.begin() + i, data.begin() + i + length));
            i += length;
            ID++;
        } else {
            throw std::runtime_error("invalid data");
        }
    }
    if (eh) {
        of = true;
        std::cout << "Overflow but to hurt you" << std::endl;
        overflow.insert(incomedata.begin(), overflow.begin(), overflow.end());
    }
    if (of) {
        std::cout << "of" << std::endl;
    }
    if ((overflow.size() > 0) && !of) {
        std::cout << "Overflow over" << std::endl;
        overflow.clear();
    }
    if (CommandID == ANCS_COMMAND_ID::GetNotificationAttributes) {
        attributes.insert(attributes.begin() + ID, NotificationUID);
    }
    if (CommandID == ANCS_COMMAND_ID::GetAppAttributes) {
        attributes.insert(attributes.begin() + ID, AppIdentaifier);
        // application_name_pause = false;
    }
    return attributes;
}

std::vector<std::string> split(std::string str, std::string delimiter) {
    std::vector<std::string> v;
    if (!str.empty()) {
        int start = 0;
        do {
            // Find the index of occurrence
            int idx = str.find(delimiter, start);
            if (idx == std::string::npos) {
                break;
            }

            // If found add the substring till that
            // occurrence in the vector
            int length = idx - start;
            v.push_back(str.substr(start, length));
            start += (length + delimiter.size());
        } while (true);
        v.push_back(str.substr(start));
    }

    return v;
}

void CANCSServer::notification_action(sdbus::Signal signal) {
    uint32_t id;
    std::string action;

    signal >> id >> action;

    std::cout << "notification_action " << action << std::endl;
    std::vector<std::string> data = split(action, " ");

    if (data.at(0) != "10f7c7") {
        return;
    }

    uint32_t stoiid = std::stoi(data.at(2));

    std::vector<int> raw_data;

    for (int i = 0; i < 4; i++) {
        raw_data.insert(raw_data.end(), ((stoiid >> (8 * i)) & 0xff));
    }

    std::cout << std::to_string(raw_data.at(0)) << " " << std::to_string(raw_data.at(1)) << " " << std::to_string(raw_data.at(2)) << " " << std::to_string(raw_data.at(3)) << std::endl;
    notification_index.erase(stoiid);
    g_pBLE->sendNotificationAction(raw_data, std::stoi(data.at(1)));
}

void CANCSServer::init() {

    CANCSServer::isDST = date::current_zone()->get_info(std::chrono::system_clock::now()).save.count();

    std::thread *ancs_server_async_thread = new std::thread(ancs_server_async_thread_function);

    proxy = sdbus::createProxy(ANCS_NOTIFICATIONS_SNAME, ANCS_NOTIFICATIONS_OPATH);

    proxy->registerSignalHandler(ANCS_NOTIFICATIONS_IFACE, ANCS_NOTIFICATIONS_SIGNAL, [this](sdbus::Signal signal) { notification_action(signal); });

    while (ancs_server_async_thread_active) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void CANCSServer::disconnectThread() {
    ancs_server_async_thread_active = false;
}
