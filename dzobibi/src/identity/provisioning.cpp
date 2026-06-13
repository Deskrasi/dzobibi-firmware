#include "provisioning.h"
#include "../config/constants.h"
#include <Preferences.h>
#include <Arduino.h>

static ProvisionedConfig s_cfg;
static Preferences prefs;

static void set_defaults(ProvisionedConfig& c) {
    strncpy(c.mqtt_broker, "", sizeof(c.mqtt_broker));
    c.mqtt_port = 8883;
    strncpy(c.mqtt_user, "", sizeof(c.mqtt_user));
    strncpy(c.mqtt_pass, "", sizeof(c.mqtt_pass));
    strncpy(c.gsm_apn,  "internet", sizeof(c.gsm_apn));
    strncpy(c.gsm_user, "", sizeof(c.gsm_user));
    strncpy(c.gsm_pass, "", sizeof(c.gsm_pass));
    strncpy(c.wifi_ssid, "", sizeof(c.wifi_ssid));
    strncpy(c.wifi_pass, "", sizeof(c.wifi_pass));
    c.co_alarm_ppm       = CO_ALARM_PPM;
    c.voc_alarm_index    = VOC_ALARM_INDEX;
    c.telemetry_interval_s = 60;
    strncpy(c.device_name, "Dzobibi", sizeof(c.device_name));
}

void load_config(ProvisionedConfig& cfg) {
    set_defaults(cfg);
    prefs.begin("dzobibi", true);
    prefs.getString("mqtt_broker", cfg.mqtt_broker, sizeof(cfg.mqtt_broker));
    cfg.mqtt_port = prefs.getUShort("mqtt_port", cfg.mqtt_port);
    prefs.getString("mqtt_user",  cfg.mqtt_user,  sizeof(cfg.mqtt_user));
    prefs.getString("mqtt_pass",  cfg.mqtt_pass,  sizeof(cfg.mqtt_pass));
    prefs.getString("gsm_apn",    cfg.gsm_apn,    sizeof(cfg.gsm_apn));
    prefs.getString("gsm_user",   cfg.gsm_user,   sizeof(cfg.gsm_user));
    prefs.getString("gsm_pass",   cfg.gsm_pass,   sizeof(cfg.gsm_pass));
    prefs.getString("wifi_ssid",  cfg.wifi_ssid,  sizeof(cfg.wifi_ssid));
    prefs.getString("wifi_pass",  cfg.wifi_pass,  sizeof(cfg.wifi_pass));
    cfg.co_alarm_ppm       = prefs.getInt("co_alarm_ppm",  cfg.co_alarm_ppm);
    cfg.voc_alarm_index    = prefs.getInt("voc_alarm_idx", cfg.voc_alarm_index);
    cfg.telemetry_interval_s = prefs.getInt("tel_interval", cfg.telemetry_interval_s);
    prefs.getString("device_name", cfg.device_name, sizeof(cfg.device_name));
    prefs.end();
    s_cfg = cfg;
}

void save_config(const ProvisionedConfig& cfg) {
    s_cfg = cfg;
    prefs.begin("dzobibi", false);
    prefs.putString("mqtt_broker", cfg.mqtt_broker);
    prefs.putUShort("mqtt_port",   cfg.mqtt_port);
    prefs.putString("mqtt_user",   cfg.mqtt_user);
    prefs.putString("mqtt_pass",   cfg.mqtt_pass);
    prefs.putString("gsm_apn",     cfg.gsm_apn);
    prefs.putString("gsm_user",    cfg.gsm_user);
    prefs.putString("gsm_pass",    cfg.gsm_pass);
    prefs.putString("wifi_ssid",   cfg.wifi_ssid);
    prefs.putString("wifi_pass",   cfg.wifi_pass);
    prefs.putInt("co_alarm_ppm",   cfg.co_alarm_ppm);
    prefs.putInt("voc_alarm_idx",  cfg.voc_alarm_index);
    prefs.putInt("tel_interval",   cfg.telemetry_interval_s);
    prefs.putString("device_name", cfg.device_name);
    prefs.end();
}

void update_thresholds(ProvisionedConfig& cfg, int co_ppm, int voc_idx, int telemetry_s) {
    if (co_ppm > 0)       cfg.co_alarm_ppm       = co_ppm;
    if (voc_idx > 0)      cfg.voc_alarm_index    = voc_idx;
    if (telemetry_s > 0)  cfg.telemetry_interval_s = telemetry_s;
    save_config(cfg);
}

const ProvisionedConfig& active_config() { return s_cfg; }
