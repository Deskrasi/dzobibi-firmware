#pragma once

// Returns the stable device ID (IMEI if available, else MAC-derived hex string).
// Must be called after cellular_init() for the IMEI path.
const char* device_id_get();
void        device_id_init(const char* imei_or_null);
