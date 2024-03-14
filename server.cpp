#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include "Player.h"
#include "MediaPlayer2.h"

// Yeah, global variable is ugly, but this is just an example and we want to access
// the concatenator instance from within the concatenate method handler to be able
// to emit signals.

int main(int argc, char* argv[]) {
    // Create D-Bus connection to the system bus and requests name on it.
    // const char* serviceName = "org.sdbuscpp.concatenator";
    auto connection = sdbus::createSessionBusConnection(AMS_MPRIS_BUS_NAME);

    // Create concatenator D-Bus object.
    // const char* objectPath = "/org/sdbuscpp/concatenator";
    // auto mpris_opath = sdbus::createObject(*connection, AMS_MPRIS_OPATH);

    Player player(*connection, AMS_MPRIS_OPATH);
    MediaPlayer2 mediaplayer2(*connection, AMS_MPRIS_OPATH);

    // g_mpris = mpris_opath.get();

    // // Register D-Bus methods and signals on the concatenator object, and exports the object.
    // // const char* interfaceName = "org.sdbuscpp.Concatenator";
    // mpris_opath->registerMethod(AMS_MPRIS_PLAYER_IFACE, "Pause", "", "", &Pause);
    // // mpris_opath->registerSignal(AMS_MPRIS_PLAYER_IFACE, "concatenated", "s");
    // mpris_opath->finishRegistration();

    // Run the I/O event loop on the bus connection.
    connection->enterEventLoop();
}