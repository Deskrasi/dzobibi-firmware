#pragma once
#include <Client.h>

bool    wifi_connect(const char* ssid, const char* pass, uint32_t timeout_ms = 15000);
bool    wifi_is_connected();
Client* wifi_get_client();  // WiFiClientSecure instance
int     wifi_rssi();
void    wifi_disconnect();
