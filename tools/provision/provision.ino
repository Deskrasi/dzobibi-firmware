// One-shot NVS provisioning sketch.
// Accepts "KEY=VALUE\n" lines over Serial, writes them to NVS,
// then signals completion. Flash once, provision, re-flash main firmware.
#include <Preferences.h>
#include <Arduino.h>

Preferences prefs;

static const char* KEYS[] = {
    "mqtt_broker", "mqtt_port", "mqtt_user", "mqtt_pass",
    "gsm_apn", "gsm_user", "gsm_pass",
    "wifi_ssid", "wifi_pass",
    "device_name",
};
static const int NUM_KEYS = sizeof(KEYS) / sizeof(KEYS[0]);

static bool is_valid_key(const String& k) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (k == KEYS[i]) return true;
    }
    return false;
}

static bool host_connected = false;

void setup() {
    Serial.begin(115200);
    delay(300);
}

void loop() {
    // Announce readiness every second until host sends something
    if (!host_connected) {
        static uint32_t last_announce = 0;
        if (millis() - last_announce >= 1000) {
            last_announce = millis();
            Serial.println("PROVISION_READY");
        }
    }
    if (!Serial.available()) return;
    host_connected = true;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    if (line == "COMMIT") {
        Serial.println("PROVISION_DONE");
        return;
    }

    if (line == "DUMP") {
        prefs.begin("dzobibi", true);
        Serial.println("# NVS contents:");
        Serial.printf("  mqtt_broker = %s\n", prefs.getString("mqtt_broker", "").c_str());
        Serial.printf("  mqtt_port   = %u\n", prefs.getUShort("mqtt_port", 8883));
        Serial.printf("  mqtt_user   = %s\n", prefs.getString("mqtt_user", "").c_str());
        Serial.printf("  mqtt_pass   = ***\n");
        Serial.printf("  gsm_apn     = %s\n", prefs.getString("gsm_apn", "").c_str());
        Serial.printf("  gsm_user    = %s\n", prefs.getString("gsm_user", "").c_str());
        Serial.printf("  device_name = %s\n", prefs.getString("device_name", "").c_str());
        prefs.end();
        return;
    }

    int sep = line.indexOf('=');
    if (sep < 1) {
        Serial.printf("ERR bad format (expected KEY=VALUE): %s\n", line.c_str());
        return;
    }

    String key = line.substring(0, sep);
    String val = line.substring(sep + 1);
    key.trim(); val.trim();

    if (!is_valid_key(key)) {
        Serial.printf("ERR unknown key: %s\n", key.c_str());
        return;
    }

    prefs.begin("dzobibi", false);
    if (key == "mqtt_port") {
        prefs.putUShort("mqtt_port", (uint16_t)val.toInt());
    } else {
        prefs.putString(key.c_str(), val);
    }
    prefs.end();

    // Mask passwords in echo
    bool is_secret = (key == "mqtt_pass" || key == "wifi_pass");
    Serial.printf("OK %s=%s\n", key.c_str(), is_secret ? "***" : val.c_str());
}
