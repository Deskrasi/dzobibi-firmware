#include "co_sensor.h"
#include "../config/pins.h"
#include <Arduino.h>

// Calibration: V_out = zero_mv + (co_ppm * mv_per_ppm)
// mv_per_ppm is negative if increasing CO lowers the amp output.
// Tune these after bench calibration; store in NVS if needed.
static float s_zero_mv     = 0.0f;
static float s_mv_per_ppm  = -0.5f;  // placeholder — calibrate on hardware

static float read_mv() {
    // Average 8 samples to reduce ADC noise
    int32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += analogReadMilliVolts(PIN_CO_SENSOR);
        delayMicroseconds(500);
    }
    return sum / 8.0f;
}

void co_sensor_init() {
    analogSetAttenuation(ADC_11db);  // 0–3.9 V range
    // Brief settle time before zeroing
    delay(100);
}

void co_sensor_zero() {
    // Capture baseline in clean air; call once sensors are warm
    s_zero_mv = read_mv();
    Serial.printf("[CO] zero = %.1f mV\n", s_zero_mv);
}

float co_sensor_read_ppm() {
    if (s_zero_mv == 0.0f) return -1.0f;  // not zeroed yet
    float mv = read_mv();
    if (mv < 0.0f) return -1.0f;
    float ppm = (mv - s_zero_mv) / s_mv_per_ppm;
    return max(0.0f, ppm);
}
