#include "comms_task.h"
#include "ipc.h"
#include "../net/transport.h"
#include "../net/mqtt_client.h"
#include "../net/topics.h"
#include "../identity/provisioning.h"
#include "../identity/device_id.h"
#include "../config/constants.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

static ProvisionedConfig* s_cfg_ptr = nullptr;

// ── MQTT incoming command handler ─────────────────────────────────────────────
static void on_mqtt_message(const char* topic, const uint8_t* payload, unsigned int len) {
    if (strcmp(topic, TOPIC_CMD) != 0) return;

    // Parse up to MQTT_BUFFER_SIZE bytes
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload, len);
    if (err) return;

    const char* cmd_str = doc["cmd"] | "";
    Command cmd = {};
    bool valid = true;

    if (strcmp(cmd_str, "silence") == 0) {
        cmd.type = CmdType::SILENCE;
    } else if (strcmp(cmd_str, "test") == 0) {
        cmd.type = CmdType::TEST;
    } else if (strcmp(cmd_str, "reboot") == 0) {
        cmd.type = CmdType::REBOOT;
    } else if (strcmp(cmd_str, "config") == 0) {
        cmd.type                   = CmdType::CONFIG;
        cmd.co_alarm_ppm           = doc["params"]["co_threshold_ppm"] | 0;
        cmd.voc_alarm_index        = doc["params"]["voc_alarm_index"] | 0;
        cmd.telemetry_interval_s   = doc["params"]["telemetry_interval_s"] | 0;
    } else {
        valid = false;
    }

    if (!valid) return;

    // Apply CONFIG here; forward others to SafetyTask
    if (cmd.type == CmdType::CONFIG && s_cfg_ptr) {
        update_thresholds(*s_cfg_ptr, cmd.co_alarm_ppm, cmd.voc_alarm_index,
                          cmd.telemetry_interval_s);
    } else {
        xQueueSend(xCmdQueue, &cmd, 0);
    }

    // Ack
    char ack[128];
    snprintf(ack, sizeof(ack), "{\"cmd\":\"%s\",\"result\":\"ok\",\"ts\":%lu}",
             cmd_str, (unsigned long)ipc_now());
    mqtt_publish(TOPIC_ACK, ack, false, 1);
}

// ── Payload builders ──────────────────────────────────────────────────────────
static void publish_alarm(const AlarmEvent& evt) {
    const char* type_str =
        (evt.type == EventType::ALARM_TEST) ? "TEST" :
        (evt.co_ppm >= CO_CRITICAL_PPM)     ? "CO"   : "FIRE";
    const char* sev_str  =
        (evt.co_ppm >= CO_CRITICAL_PPM) ? "critical" : "warning";

    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"type\":\"%s\",\"severity\":\"%s\","
        "\"co_ppm\":%.1f,\"voc_index\":%ld}",
        (unsigned long)evt.ts, type_str, sev_str,
        evt.co_ppm, (long)evt.voc_index);
    mqtt_publish(TOPIC_ALARM, buf, false, 1);
}

static void publish_telemetry() {
    float    bat_v   = 0.0f;
    uint8_t  bat_pct = 0;
    float    co_ppm  = 0.0f;
    int32_t  voc_idx = 0;
    int32_t  nox_idx = 0;

    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        bat_v   = g_state.battery_v;
        bat_pct = g_state.battery_pct;
        co_ppm  = g_state.co_ppm;
        voc_idx = g_state.voc_index;
        nox_idx = g_state.nox_index;
        xSemaphoreGive(g_state.mutex);
    }

    char buf[320];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"battery_v\":%.2f,\"battery_pct\":%u,"
        "\"co_ppm\":%.1f,\"voc_index\":%ld,\"nox_index\":%ld,"
        "\"rssi\":%d,\"link\":\"%s\"}",
        (unsigned long)ipc_now(), bat_v, bat_pct,
        co_ppm, (long)voc_idx, (long)nox_idx,
        transport_rssi(), transport_link_name());
    mqtt_publish(TOPIC_TELEMETRY, buf, false, 0);
}

// ── CommsTask ─────────────────────────────────────────────────────────────────
void comms_task(void* pv) {
    esp_task_wdt_add(NULL);

    s_cfg_ptr = (ProvisionedConfig*)pv;
    const ProvisionedConfig& cfg = *s_cfg_ptr;

    // If no broker provisioned, comms task idles (local alarm still works)
    if (strlen(cfg.mqtt_broker) == 0) {
        Serial.println("[COMMS] no broker provisioned — running local-only");
        for (;;) {
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    // Connect transport
    while (!transport_connect()) {
        Serial.println("[COMMS] transport not available, retrying in 10 s...");
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

    // LWT payload
    char lwt[64];
    snprintf(lwt, sizeof(lwt), "{\"online\":false,\"device_id\":\"%s\"}", device_id_get());

    MqttConfig mcfg = {
        cfg.mqtt_broker, cfg.mqtt_port,
        cfg.mqtt_user,   cfg.mqtt_pass,
        device_id_get(),
        TOPIC_STATUS,    lwt,
    };
    mqtt_begin(mcfg, on_mqtt_message);

    uint32_t last_telemetry_ms = 0;
    uint32_t telemetry_interval_ms = (uint32_t)cfg.telemetry_interval_s * 1000;

    for (;;) {
        esp_task_wdt_reset();
        uint32_t now = millis();

        transport_maintain();

        if (mqtt_ensure_connected()) {
            mqtt_loop();

            // Drain alarm event queue and publish (QoS 1)
            AlarmEvent evt;
            while (xQueueReceive(xAlarmEventQueue, &evt, 0) == pdTRUE) {
                publish_alarm(evt);
            }

            // Periodic telemetry
            if ((now - last_telemetry_ms) >= telemetry_interval_ms) {
                last_telemetry_ms = now;
                publish_telemetry();

                // Update RSSI in shared state
                if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_state.rssi          = transport_rssi();
                    g_state.mqtt_connected = true;
                    xSemaphoreGive(g_state.mutex);
                }
            }
        } else {
            if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                g_state.mqtt_connected = false;
                xSemaphoreGive(g_state.mutex);
            }
        }

        // Re-read config interval in case cmd:config changed it
        telemetry_interval_ms = (uint32_t)active_config().telemetry_interval_s * 1000;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
