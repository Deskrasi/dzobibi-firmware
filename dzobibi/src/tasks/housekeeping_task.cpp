#include "housekeeping_task.h"
#include "ipc.h"
#include "../power/battery.h"
#include "../alarm/buzzer.h"
#include "../config/constants.h"
#include <Arduino.h>
#include <esp_task_wdt.h>

void housekeeping_task(void* pv) {
    esp_task_wdt_add(NULL);

    battery_init();

    uint32_t last_batt_ms  = 0;
    bool     low_bat_warned = false;

    for (;;) {
        esp_task_wdt_reset();
        uint32_t now = millis();

        if ((now - last_batt_ms) >= BATT_MEASURE_MS) {
            last_batt_ms = now;

            float   bat_v   = battery_voltage_v();
            uint8_t bat_pct = battery_percent();

            if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                g_state.battery_v   = bat_v;
                g_state.battery_pct = bat_pct;
                xSemaphoreGive(g_state.mutex);
            }

            if (battery_is_critical()) {
                if (!low_bat_warned) {
                    Serial.printf("[POWER] CRITICAL battery: %.2f V (%d%%)\n", bat_v, bat_pct);
                    // Chirp warning (non-intrusive)
                    buzzer_set(BuzzerPattern::CHIRP);
                    low_bat_warned = true;
                }
            } else if (battery_is_low()) {
                Serial.printf("[POWER] low battery: %.2f V (%d%%)\n", bat_v, bat_pct);
                low_bat_warned = false;
            } else {
                low_bat_warned = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
