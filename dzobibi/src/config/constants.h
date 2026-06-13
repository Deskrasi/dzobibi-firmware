#pragma once
#include <stdint.h>

#define FW_VERSION  "1.0.0"

// ── CO alarm thresholds (ppm) ─────────────────────────────────────────────
#define CO_WARNING_PPM      50
#define CO_ALARM_PPM        100
#define CO_CRITICAL_PPM     300   // alarm with zero debounce

// ── VOC alarm (Sensirion VOC Index: 100 = clean-air baseline, 0-500) ─────
#define VOC_ALARM_INDEX     300
#define VOC_RISE_DELTA      100   // index delta per sample = rapid-rise trigger

// ── Alarm debounce ────────────────────────────────────────────────────────
#define ALARM_DEBOUNCE      3     // consecutive confirming samples before latch

// ── Silence / hush timeout ────────────────────────────────────────────────
#define ALARM_HUSH_MS       (5 * 60 * 1000UL)

// ── Test beep duration ────────────────────────────────────────────────────
#define ALARM_TEST_MS       3000UL

// ── Sensor sample interval ────────────────────────────────────────────────
#define CO_SAMPLE_MS        500UL           // 2 Hz
#define GAS_SAMPLE_MS       1000UL          // SGP41 max rate = 1 Hz
#define SGP41_COND_ROUNDS   10              // NOx conditioning rounds

// ── Telemetry ─────────────────────────────────────────────────────────────
#define TELEMETRY_INTERVAL_MS    (60 * 1000UL)
#define TELEMETRY_LOW_BAT_MS     (5 * 60 * 1000UL)  // throttle when battery low

// ── Battery (mV measured after ×2) ───────────────────────────────────────
#define BATT_FULL_MV        4200
#define BATT_EMPTY_MV       3000
#define BATT_LOW_MV         3400
#define BATT_CRITICAL_MV    3100
#define BATT_MEASURE_MS     (30 * 1000UL)

// ── MQTT ─────────────────────────────────────────────────────────────────
#define MQTT_KEEPALIVE_S        60
#define MQTT_BACKOFF_MIN_MS     2000UL
#define MQTT_BACKOFF_MAX_MS     (60 * 1000UL)
#define MQTT_BUFFER_SIZE        512

// ── Silence button debounce ───────────────────────────────────────────────
#define BTN_DEBOUNCE_MS     50
#define BTN_LONG_PRESS_MS   3000

// ── FreeRTOS ──────────────────────────────────────────────────────────────
#define STACK_SAFETY        4096
#define STACK_COMMS         16384
#define STACK_HOUSEKEEPING  3072

#define PRI_SAFETY          3
#define PRI_COMMS           2
#define PRI_HOUSEKEEPING    1

#define QUEUE_ALARM_DEPTH   8
#define QUEUE_CMD_DEPTH     4

// ── Watchdog timeouts (seconds) ───────────────────────────────────────────
#define WDT_SAFETY_S        10
#define WDT_COMMS_S         90
#define WDT_HOUSEKEEPING_S  60
