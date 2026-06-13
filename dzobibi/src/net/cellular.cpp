#include "cellular.h"
#include "../config/pins.h"
#include <Arduino.h>

// TinyGSM for SIM7600
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#include <TinyGsmClient.h>

static TinyGsm       s_modem(Serial1);
static TinyGsmClient s_client(s_modem, 0);
// TinyGSM's SIM7600 driver has no TLS client wrapper — SSL is negotiated
// by configuring the modem's internal SSL context (AT+CSSLCFG) before
// opening the socket.  cellular_init() sets up sslversion + ignorelocaltime
// so the SIM7600 handles TLS termination transparently on the TCP socket
// that TinyGsmClient wraps.  No change needed in the PubSubClient layer.
static bool          s_data_up = false;

// Pulse PWRKEY to toggle modem power (HIGH = pull PWRKEY low)
static void pulse_pwrkey() {
    digitalWrite(PIN_MODEM_PWRKEY, HIGH);
    delay(1200);
    digitalWrite(PIN_MODEM_PWRKEY, LOW);
}

static bool modem_responsive() {
    return s_modem.testAT(2000);
}

bool cellular_init() {
    pinMode(PIN_MODEM_PWRKEY, OUTPUT);
    digitalWrite(PIN_MODEM_PWRKEY, LOW);
    Serial1.begin(115200, SERIAL_8N1, PIN_MODEM_RX, PIN_MODEM_TX);
    delay(100);

    if (!modem_responsive()) {
        Serial.println("[GSM] powering on...");
        pulse_pwrkey();
        // Wait up to 15 s for boot
        uint32_t t = millis();
        while (!modem_responsive() && (millis() - t) < 15000) delay(500);
    }

    if (!modem_responsive()) {
        Serial.println("[GSM] no response after power-on");
        return false;
    }

    s_modem.sendAT("+CMEE=2");      // verbose errors
    s_modem.waitResponse();
    s_modem.sendAT("E0");           // echo off
    s_modem.waitResponse();

    // TLS: disable cert verification for initial bring-up
    // TODO: load HiveMQ CA cert and enable verification before production
    s_modem.sendAT("+CSSLCFG=\"ignorelocaltime\",1,1");
    s_modem.waitResponse();

    Serial.println("[GSM] modem ready");
    return true;
}

bool cellular_connect(const char* apn, const char* user, const char* pass) {
    Serial.printf("[GSM] connecting GPRS on APN '%s'...\n", apn);
    if (!s_modem.waitForNetwork(60000)) {
        Serial.println("[GSM] network registration timeout");
        return false;
    }
    s_data_up = s_modem.gprsConnect(apn, user, pass);
    if (s_data_up) Serial.println("[GSM] data up");
    else           Serial.println("[GSM] GPRS connect failed");
    return s_data_up;
}

bool cellular_is_connected() {
    return s_data_up && s_modem.isGprsConnected();
}

Client* cellular_get_client() { return &s_client; }

int cellular_rssi() {
    return s_modem.getSignalQuality();  // 0-31, 99 = unknown
}

bool cellular_get_imei(char* out, size_t len) {
    String imei = s_modem.getIMEI();
    if (imei.length() < 14) return false;
    strncpy(out, imei.c_str(), len - 1);
    out[len - 1] = '\0';
    return true;
}

void cellular_disconnect() {
    s_modem.gprsDisconnect();
    s_data_up = false;
}

void cellular_power_off() {
    s_modem.poweroff();
    s_data_up = false;
}
