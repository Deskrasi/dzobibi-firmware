// Dzobibi Rev A — integrated product firmware
// Board: ESP32S3 Dev Module | USB CDC On Boot: Enabled | USB Mode: HW CDC+JTAG
// Required libraries: TinyGSM, PubSubClient, ArduinoJson,
//                     Sensirion I2C SGP41, Sensirion Gas Index Algorithm,
//                     Adafruit NeoPixel (optional, Rev A best-effort LED)
#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "src/config/pins.h"
#include "src/config/constants.h"
#include "src/identity/device_id.h"
#include "src/identity/provisioning.h"
#include "src/net/cellular.h"
#include "src/net/topics.h"
#include "src/tasks/ipc.h"
#include "src/tasks/safety_task.h"
#include "src/tasks/comms_task.h"
#include "src/tasks/housekeeping_task.h"

static ProvisionedConfig g_cfg;

void setup() {
    Serial.begin(115200);
    delay(2000);  // Allow USB CDC to re-enumerate after flash reset
    Serial.printf("\n=== Dzobibi firmware v%s (MAC: %s) ===\n",
                  FW_VERSION, WiFi.macAddress().c_str());

    // ── Watchdog ────────────────────────────────────────────────────────────
    // Tasks register themselves; setup() does not feed the WDT.
    esp_task_wdt_config_t wdt_cfg = {
        .timeout_ms = WDT_SAFETY_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    esp_task_wdt_reconfigure(&wdt_cfg);

    // ── IPC: queues + shared state ──────────────────────────────────────────
    ipc_init();

    // ── Load NVS config ─────────────────────────────────────────────────────
    load_config(g_cfg);

    // ── Device identity ─────────────────────────────────────────────────────
    // Try IMEI first (modem may not be powered yet; fall back to MAC)
    char imei[20] = "";
    if (cellular_init() && cellular_get_imei(imei, sizeof(imei))) {
        device_id_init(imei);
    } else {
        device_id_init(nullptr);  // MAC fallback
    }
    topics_init(device_id_get());

    // ── Create FreeRTOS tasks ────────────────────────────────────────────────
    // SafetyTask runs on core 1 (same as Arduino loop) for guaranteed priority;
    // Comms and Housekeeping can run on either core.
    xTaskCreatePinnedToCore(safety_task,       "SafetyTask",       STACK_SAFETY,
                             nullptr,           PRI_SAFETY,   nullptr, 1);
    xTaskCreatePinnedToCore(comms_task,        "CommsTask",        STACK_COMMS,
                             (void*)&g_cfg,    PRI_COMMS,    nullptr, 0);
    xTaskCreatePinnedToCore(housekeeping_task, "HousekeepingTask", STACK_HOUSEKEEPING,
                             nullptr,           PRI_HOUSEKEEPING, nullptr, 0);

    Serial.println("[BOOT] tasks created — entering FreeRTOS scheduler");
}

// loop() is intentionally empty; all work runs in FreeRTOS tasks.
void loop() {
    vTaskDelay(portMAX_DELAY);
}
