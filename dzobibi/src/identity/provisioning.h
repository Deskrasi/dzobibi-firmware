#pragma once
#include <stdint.h>

struct ProvisionedConfig {
    char     mqtt_broker[64];
    uint16_t mqtt_port;
    char     mqtt_user[32];
    char     mqtt_pass[64];
    char     gsm_apn[32];
    char     gsm_user[16];
    char     gsm_pass[16];
    char     wifi_ssid[32];
    char     wifi_pass[64];
    int      co_alarm_ppm;
    int      voc_alarm_index;
    int      telemetry_interval_s;
    char     device_name[32];
};

void load_config(ProvisionedConfig& cfg);
void save_config(const ProvisionedConfig& cfg);

// Apply partial updates from cmd:config JSON (co_alarm_ppm, voc_alarm_index,
// telemetry_interval_s). Pass 0 to leave a value unchanged.
void update_thresholds(ProvisionedConfig& cfg, int co_ppm, int voc_idx, int telemetry_s);

// Expose live config pointer (read-only outside provisioning module)
const ProvisionedConfig& active_config();
