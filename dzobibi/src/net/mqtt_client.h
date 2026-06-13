#pragma once
#include <stdint.h>

struct MqttConfig {
    const char* broker;
    uint16_t    port;
    const char* username;
    const char* password;
    const char* device_id;
    const char* lwt_topic;    // TOPIC_STATUS
    const char* lwt_payload;  // JSON {"online":false}
};

using MqttCallback = void (*)(const char* topic, const uint8_t* payload, unsigned int len);

bool mqtt_begin(const MqttConfig& cfg, MqttCallback cb);
bool mqtt_ensure_connected();     // reconnect with exponential backoff if needed
void mqtt_loop();                 // must be called regularly from CommsTask
bool mqtt_publish(const char* topic, const char* payload, bool retained = false, uint8_t qos = 0);
bool mqtt_subscribe(const char* topic);
bool mqtt_is_connected();
void mqtt_disconnect();
