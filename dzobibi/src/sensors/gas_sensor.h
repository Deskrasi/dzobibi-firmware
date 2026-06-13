#pragma once
#include <stdint.h>

struct GasReading {
    int32_t voc_index;   // 0-500; 100 = clean-air baseline
    int32_t nox_index;   // 0-500; 1 = clean-air baseline
    bool    valid;       // false during SGP41 conditioning window
};

bool        gas_sensor_init();      // returns false if SGP41 not found
GasReading  gas_sensor_read();
