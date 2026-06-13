#pragma once
// OTA update support — planned for post-MVP.
// Triggered by cmd:ota_url over MQTT; pulls firmware image over HTTPS
// and uses ESP32 OTA partition scheme. Requires OTA-capable partition table.

void ota_begin();
bool ota_start_from_url(const char* url);  // blocking; reboots on success
