#pragma once
#include <Client.h>

// Returns the active TLS-capable Client (cellular or WiFi).
// Callers (MQTT) use this without knowing which link is live.
Client* transport_get_client();

bool        transport_connect();          // brings up whichever link is available
bool        transport_is_connected();
void        transport_maintain();         // call from CommsTask loop
int         transport_rssi();             // dBm or raw signal value
const char* transport_link_name();        // "cellular" | "wifi" | "none"
void        transport_disconnect();
