#pragma once
#include <Client.h>

bool    cellular_init();          // power-on modem (detect first, then pulse)
bool    cellular_connect(const char* apn, const char* user, const char* pass);
bool    cellular_is_connected();
Client* cellular_get_client();    // TinyGsmClientSecure instance
int     cellular_rssi();          // 0-31 (AT+CSQ), 99 = unknown
bool    cellular_get_imei(char* out, size_t len);
void    cellular_disconnect();
void    cellular_power_off();
