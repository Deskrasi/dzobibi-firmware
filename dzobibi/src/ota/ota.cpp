#include "ota.h"
#include <Arduino.h>

void ota_begin() {
    // TODO: initialise Update library and set OTA partition
}

bool ota_start_from_url(const char* url) {
    Serial.printf("[OTA] update from %s — not yet implemented\n", url);
    return false;
}
