#include "transport.h"
#include "cellular.h"
#include "wifi_net.h"
#include "../identity/provisioning.h"
#include <Arduino.h>

static enum class Link { NONE, CELLULAR, WIFI } s_link = Link::NONE;

bool transport_connect() {
    const ProvisionedConfig& cfg = active_config();

    // WiFi first when credentials are provisioned.
    // ESP32's WiFiClientSecure provides native TLS — required for HiveMQ Cloud.
    // Cellular (TinyGsmClient) uses plain TCP; TLS over cellular needs a
    // SIM7600 firmware update to enable AT+CCHOPEN (not supported on LE20B04).
    if (strlen(cfg.wifi_ssid) > 0 &&
        wifi_connect(cfg.wifi_ssid, cfg.wifi_pass)) {
        s_link = Link::WIFI;
        return true;
    }

    // Cellular fallback (plain TCP only on current firmware)
    if (cellular_init() && cellular_connect(cfg.gsm_apn, cfg.gsm_user, cfg.gsm_pass)) {
        s_link = Link::CELLULAR;
        return true;
    }

    s_link = Link::NONE;
    return false;
}

bool transport_is_connected() {
    switch (s_link) {
        case Link::CELLULAR: return cellular_is_connected();
        case Link::WIFI:     return wifi_is_connected();
        default:             return false;
    }
}

void transport_maintain() {
    if (!transport_is_connected()) {
        Serial.println("[TRANSPORT] link lost, reconnecting...");
        s_link = Link::NONE;
        transport_connect();
    }
}

int transport_rssi() {
    switch (s_link) {
        case Link::CELLULAR: return cellular_rssi();
        case Link::WIFI:     return wifi_rssi();
        default:             return 0;
    }
}

const char* transport_link_name() {
    switch (s_link) {
        case Link::CELLULAR: return "cellular";
        case Link::WIFI:     return "wifi";
        default:             return "none";
    }
}

Client* transport_get_client() {
    switch (s_link) {
        case Link::CELLULAR: return cellular_get_client();
        case Link::WIFI:     return wifi_get_client();
        default:             return nullptr;
    }
}

void transport_disconnect() {
    cellular_disconnect();
    wifi_disconnect();
    s_link = Link::NONE;
}
