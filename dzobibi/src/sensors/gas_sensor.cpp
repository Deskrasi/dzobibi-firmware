#include "gas_sensor.h"
#include "../config/pins.h"
#include "../config/constants.h"
#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>

static SensirionI2CSgp41    sgp41;
static VOCGasIndexAlgorithm vocAlgo;
static NOxGasIndexAlgorithm noxAlgo;
static int s_cond_count = 0;
static bool s_available = false;

// Default compensation (50 % RH, 25 °C) in SGP41 fixed-point format
static const uint16_t DEF_RH = 0x8000;
static const uint16_t DEF_T  = 0x6666;

bool gas_sensor_init() {
    Wire.begin(PIN_SDA, PIN_SCL);
    sgp41.begin(Wire);
    uint16_t err;
    uint16_t testResult;
    err = sgp41.executeSelfTest(testResult);
    if (err || testResult != 0xD400) {
        Serial.printf("[SGP41] self-test failed: err=%u result=0x%04X\n", err, testResult);
        return false;
    }
    s_available = true;
    Serial.println("[SGP41] ready");
    return true;
}

GasReading gas_sensor_read() {
    GasReading r = {100, 1, false};
    if (!s_available) return r;

    uint16_t sraw_voc, sraw_nox;
    uint16_t err;

    if (s_cond_count < SGP41_COND_ROUNDS) {
        // NOx conditioning phase — only VOC channel runs
        err = sgp41.executeConditioning(DEF_RH, DEF_T, sraw_voc);
        if (err) { Serial.printf("[SGP41] conditioning err=%u\n", err); return r; }
        r.voc_index = (int32_t)vocAlgo.process(sraw_voc);
        r.nox_index = 1;
        r.valid     = false;
        s_cond_count++;
        return r;
    }

    err = sgp41.measureRawSignals(DEF_RH, DEF_T, sraw_voc, sraw_nox);
    if (err) { Serial.printf("[SGP41] read err=%u\n", err); return r; }

    r.voc_index = (int32_t)vocAlgo.process(sraw_voc);
    r.nox_index = (int32_t)noxAlgo.process(sraw_nox);
    r.valid     = true;
    return r;
}
