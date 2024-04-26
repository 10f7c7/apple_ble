#include "ANCSServer.hpp"

std::string CANCSServer::transferData(ANCS_NOTIF_SRC_ATTR ancs_notif_src) {
    std::cout << "transferData" << std::endl;
    std::cout << "EventID: " << ancs_notif_src.NotificationUIDDec << std::endl;
    std::cout << "EventFlags: " << (int)ancs_notif_src.EventFlags << std::endl;
    notification_index.insert({ ancs_notif_src.NotificationUIDDec, ancs_notif_src.EventFlags });
    if (ancs_notif_src.EventID == 0x02) {
        return "";
    }
    bool preexist = ancs_notif_src.EventFlags & (1 << NOTIF_FLAG::PREEXISTING);

    std::vector<uint8_t> action;

    if (ancs_notif_src.EventFlags & (1 << NOTIF_FLAG::POSITIVE_ACTION)) {
        action.insert(action.end(), 0x06);
    }
    else if (ancs_notif_src.EventFlags & (1 << NOTIF_FLAG::NEGATIVE_ACTION)) {
        action.insert(action.end(), 0x07);
    }

    g_pBLE->sendNotification(ancs_notif_src, action);
    

    return ""; // Placeholder return value
}