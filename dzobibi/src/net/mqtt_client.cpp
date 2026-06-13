#include "mqtt_client.h"
#include "transport.h"
#include "topics.h"
#include "../config/constants.h"
#include <Arduino.h>
#include <PubSubClient.h>

static PubSubClient  s_mqtt;
static MqttConfig    s_cfg;
static MqttCallback  s_cb;
static uint32_t      s_backoff_ms      = MQTT_BACKOFF_MIN_MS;
static uint32_t      s_last_attempt_ms = 0;

static void on_message(char* topic, byte* payload, unsigned int len) {
    if (s_cb) s_cb(topic, payload, len);
}

bool mqtt_begin(const MqttConfig& cfg, MqttCallback cb) {
    s_cfg = cfg;
    s_cb  = cb;
    Client* c = transport_get_client();
    if (!c) return false;
    s_mqtt.setClient(*c);
    s_mqtt.setServer(cfg.broker, cfg.port);
    s_mqtt.setKeepAlive(MQTT_KEEPALIVE_S);
    s_mqtt.setBufferSize(MQTT_BUFFER_SIZE);
    s_mqtt.setCallback(on_message);
    return true;
}

static bool do_connect() {
    if (!transport_is_connected()) return false;

    // Rebuild the PubSubClient client pointer in case transport reconnected
    Client* c = transport_get_client();
    if (!c) return false;
    s_mqtt.setClient(*c);
    s_mqtt.setServer(s_cfg.broker, s_cfg.port);

    char client_id[32];
    snprintf(client_id, sizeof(client_id), "dzobibi-%s", s_cfg.device_id);

    // LWT: retained "offline" status
    bool ok = s_mqtt.connect(
        client_id,
        s_cfg.username,
        s_cfg.password,
        s_cfg.lwt_topic,
        1,      // QoS 1
        true,   // retained
        s_cfg.lwt_payload
    );

    if (ok) {
        Serial.printf("[MQTT] connected to %s\n", s_cfg.broker);
        s_mqtt.subscribe(TOPIC_CMD, 1);
        s_backoff_ms = MQTT_BACKOFF_MIN_MS;

        // Publish "online" retained status
        char status[128];
        snprintf(status, sizeof(status),
            "{\"online\":true,\"fw\":\"%s\",\"link\":\"%s\"}",
            FW_VERSION, transport_link_name());
        s_mqtt.publish(s_cfg.lwt_topic, (const uint8_t*)status, strlen(status), true);
    } else {
        Serial.printf("[MQTT] connect failed, rc=%d, backoff=%lu ms\n",
                      s_mqtt.state(), s_backoff_ms);
    }
    return ok;
}

bool mqtt_ensure_connected() {
    if (s_mqtt.connected()) return true;

    uint32_t now = millis();
    if ((now - s_last_attempt_ms) < s_backoff_ms) return false;

    s_last_attempt_ms = now;
    bool ok = do_connect();
    if (!ok) {
        s_backoff_ms = min(s_backoff_ms * 2, MQTT_BACKOFF_MAX_MS);
    }
    return ok;
}

void mqtt_loop() {
    if (s_mqtt.connected()) s_mqtt.loop();
}

bool mqtt_publish(const char* topic, const char* payload, bool retained, uint8_t qos) {
    if (!s_mqtt.connected()) return false;
    if (qos == 1) {
        return s_mqtt.publish(topic, (const uint8_t*)payload, strlen(payload), retained);
    }
    return s_mqtt.publish(topic, payload, retained);
}

bool mqtt_subscribe(const char* topic) {
    return s_mqtt.connected() && s_mqtt.subscribe(topic, 1);
}

bool mqtt_is_connected() { return s_mqtt.connected(); }

void mqtt_disconnect() { s_mqtt.disconnect(); }
