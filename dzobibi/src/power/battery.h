#pragma once
#include <stdint.h>

void    battery_init();
float   battery_voltage_v();   // pulses BAT_MEASURE, reads ADC, returns volts
uint8_t battery_percent();     // 0-100 (linear approximation, good enough)
bool    battery_is_low();      // < BATT_LOW_MV
bool    battery_is_critical(); // < BATT_CRITICAL_MV
