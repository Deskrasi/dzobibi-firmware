#include "battery.h"
#include "../config/pins.h"
#include "../config/constants.h"
#include <Arduino.h>

void battery_init() {
    pinMode(PIN_BAT_MEASURE, OUTPUT);
    digitalWrite(PIN_BAT_MEASURE, LOW);
}

float battery_voltage_v() {
    // Enable divider, settle, read, disable
    digitalWrite(PIN_BAT_MEASURE, HIGH);
    delay(5);
    int32_t sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += analogReadMilliVolts(PIN_BATT_SENSE);
        delay(2);
    }
    digitalWrite(PIN_BAT_MEASURE, LOW);
    // ×2 because the divider halves the battery voltage
    return (sum / 4.0f * 2.0f) / 1000.0f;
}

uint8_t battery_percent() {
    float mv = battery_voltage_v() * 1000.0f;
    int clamped = constrain((int)mv, BATT_EMPTY_MV, BATT_FULL_MV);
    return (uint8_t)map(clamped, BATT_EMPTY_MV, BATT_FULL_MV, 0, 100);
}

bool battery_is_low()     { return battery_voltage_v() * 1000.0f < BATT_LOW_MV; }
bool battery_is_critical(){ return battery_voltage_v() * 1000.0f < BATT_CRITICAL_MV; }
