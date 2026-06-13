#include "wifi_net.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

static WiFiClientSecure s_client;
static bool             s_connected = false;

bool wifi_connect(const char* ssid, const char* pass, uint32_t timeout_ms) {
    if (!ssid || strlen(ssid) == 0) return false;
    Serial.printf("[WiFi] connecting to '%s'...\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t) < timeout_ms) {
        delay(250);
    }
    s_connected = (WiFi.status() == WL_CONNECTED);
    if (s_connected) {
        Serial.printf("[WiFi] connected, IP=%s\n", WiFi.localIP().toString().c_str());
        // TODO: load HiveMQ CA cert and call s_client.setCACert(...)
        s_client.setInsecure();  // development only
    }
    return s_connected;
}

bool    wifi_is_connected() { return s_connected && WiFi.status() == WL_CONNECTED; }
Client* wifi_get_client()   { return &s_client; }
int     wifi_rssi()         { return WiFi.RSSI(); }
void    wifi_disconnect()   { WiFi.disconnect(); s_connected = false; }
