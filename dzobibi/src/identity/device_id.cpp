#include "device_id.h"
#include <Arduino.h>
#include <esp_efuse.h>
#include <esp_mac.h>

static char s_id[20] = "";

void device_id_init(const char* imei_or_null) {
    if (imei_or_null && strlen(imei_or_null) >= 14) {
        strncpy(s_id, imei_or_null, sizeof(s_id) - 1);
    } else {
        // Fallback: derive from Wi-Fi station MAC
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        snprintf(s_id, sizeof(s_id), "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    Serial.printf("[ID] device_id = %s\n", s_id);
}

const char* device_id_get() { return s_id; }
