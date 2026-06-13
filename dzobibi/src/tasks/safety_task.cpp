#include "safety_task.h"
#include "ipc.h"
#include "../alarm/alarm.h"
#include "../sensors/co_sensor.h"
#include "../sensors/gas_sensor.h"
#include "../config/pins.h"
#include "../config/constants.h"
#include <Arduino.h>
#include <esp_task_wdt.h>

// ── Button debounce ──────────────────────────────────────────────────────────
static bool   s_btn_last       = HIGH;
static uint32_t s_btn_ms       = 0;
static uint32_t s_btn_held_ms  = 0;
static bool   s_btn_handled    = false;

static void handle_button() {
    bool cur = digitalRead(PIN_SILENCE_BTN);  // active-low
    uint32_t now = millis();

    if (cur != s_btn_last) {
        if ((now - s_btn_ms) >= BTN_DEBOUNCE_MS) {
            s_btn_last = cur;
            s_btn_ms   = now;
            if (cur == LOW) {
                // Press start
                s_btn_held_ms = now;
                s_btn_handled = false;
            } else {
                // Release
                if (!s_btn_handled) {
                    // Short press: silence
                    alarm_silence();
                }
            }
        }
    }

    // Long press while held: test alarm
    if (cur == LOW && !s_btn_handled &&
        (now - s_btn_held_ms) >= BTN_LONG_PRESS_MS) {
        alarm_test();
        s_btn_handled = true;
    }
}

// ── Safety task ──────────────────────────────────────────────────────────────
void safety_task(void* pv) {
    esp_task_wdt_add(NULL);

    alarm_init();
    co_sensor_init();
    bool gas_ok = gas_sensor_init();
    pinMode(PIN_SILENCE_BTN, INPUT);  // external pull-up already fitted

    // Give sensors time to warm up then capture CO zero reference
    vTaskDelay(pdMS_TO_TICKS(2000));
    co_sensor_zero();

    AlarmState prev_state   = AlarmState::IDLE;
    int32_t    voc_prev     = 100;   // SGP41 clean-air baseline
    uint32_t   last_co_ms   = 0;
    uint32_t   last_gas_ms  = 0;
    float      co_ppm       = 0.0f;
    GasReading gas          = {100, 1, false};

    for (;;) {
        esp_task_wdt_reset();
        uint32_t now = millis();

        // Sample CO sensor at CO_SAMPLE_MS rate
        if ((now - last_co_ms) >= CO_SAMPLE_MS) {
            last_co_ms = now;
            float raw = co_sensor_read_ppm();
            if (raw >= 0.0f) co_ppm = raw;
        }

        // Sample SGP41 at GAS_SAMPLE_MS rate
        if (gas_ok && (now - last_gas_ms) >= GAS_SAMPLE_MS) {
            last_gas_ms = now;
            GasReading r = gas_sensor_read();
            if (r.valid) {
                voc_prev = gas.voc_index;
                gas      = r;
            }
        }

        // Process silence/test button
        handle_button();

        // Drain commands from CommsTask
        Command cmd;
        while (xQueueReceive(xCmdQueue, &cmd, 0) == pdTRUE) {
            switch (cmd.type) {
                case CmdType::SILENCE: alarm_silence(); break;
                case CmdType::TEST:    alarm_test();    break;
                case CmdType::REBOOT:  esp_restart();   break;
                default: break;
            }
        }

        // Run alarm state machine
        AlarmInputs inputs = {co_ppm, gas.voc_index, voc_prev, gas.nox_index};
        bool changed = alarm_update(inputs);
        alarm_tick();

        AlarmState cur = alarm_state();

        // Push event to CommsTask on state transitions
        if (changed) {
            AlarmEvent evt = {};
            evt.co_ppm    = co_ppm;
            evt.voc_index = gas.voc_index;
            evt.nox_index = gas.nox_index;
            evt.ts        = ipc_now();

            if (cur == AlarmState::ACTIVE) {
                evt.type = EventType::ALARM_TRIGGER;
                xQueueSend(xAlarmEventQueue, &evt, 0);  // non-blocking; safe path never blocks
            } else if (prev_state == AlarmState::ACTIVE && cur == AlarmState::IDLE) {
                evt.type = EventType::ALARM_CLEAR;
                xQueueSend(xAlarmEventQueue, &evt, 0);
            } else if (cur == AlarmState::TEST) {
                evt.type = EventType::ALARM_TEST;
                xQueueSend(xAlarmEventQueue, &evt, 0);
            }
        }
        prev_state = cur;

        // Update shared state for telemetry
        if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            g_state.co_ppm       = co_ppm;
            g_state.voc_index    = gas.voc_index;
            g_state.nox_index    = gas.nox_index;
            g_state.alarm_state  = cur;
            xSemaphoreGive(g_state.mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(50));  // 20 Hz polling; buzzer tick stays responsive
    }
}
