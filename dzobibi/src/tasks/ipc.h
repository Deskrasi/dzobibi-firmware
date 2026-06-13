#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "../alarm/alarm.h"

// ── SafetyTask → CommsTask ────────────────────────────────────────────────
enum class EventType : uint8_t { ALARM_TRIGGER, ALARM_CLEAR, ALARM_TEST };

struct AlarmEvent {
    EventType type;
    float     co_ppm;
    int32_t   voc_index;
    int32_t   nox_index;
    uint32_t  ts;          // unix timestamp (or millis()/1000 if time unsynced)
};

// ── CommsTask → SafetyTask / HousekeepingTask ─────────────────────────────
enum class CmdType : uint8_t { SILENCE, TEST, CONFIG, REBOOT };

struct Command {
    CmdType type;
    int     co_alarm_ppm;
    int     voc_alarm_index;
    int     telemetry_interval_s;
};

// ── Shared sensor state (polled by CommsTask for telemetry) ───────────────
struct SharedState {
    SemaphoreHandle_t mutex;
    float     co_ppm;
    int32_t   voc_index;
    int32_t   nox_index;
    float     battery_v;
    uint8_t   battery_pct;
    AlarmState alarm_state;
    int       rssi;
    bool      mqtt_connected;
    uint32_t  boot_epoch;   // UTC epoch captured at time sync
    uint32_t  boot_millis;  // millis() at the moment boot_epoch was set
};

extern QueueHandle_t xAlarmEventQueue;
extern QueueHandle_t xCmdQueue;
extern SharedState   g_state;

void    ipc_init();
uint32_t ipc_now();  // best-effort UTC timestamp; falls back to millis()/1000
