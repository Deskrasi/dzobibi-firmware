// One-shot SIM diagnostic. Sends key AT commands and prints results.
// Flash, read output, then re-flash main firmware.
#include <Arduino.h>

#define MODEM_TX  43
#define MODEM_RX  44
#define MODEM_PWR 35

static String at(const char* cmd, uint32_t wait_ms = 2000) {
    Serial1.println(cmd);
    uint32_t t = millis();
    String r;
    while (millis() - t < wait_ms) {
        while (Serial1.available()) r += (char)Serial1.read();
    }
    r.trim();
    return r;
}

static void print_at(const char* label, const char* cmd, uint32_t ms = 2000) {
    String r = at(cmd, ms);
    Serial.printf("%-20s %s\n", label, r.c_str());
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    pinMode(MODEM_PWR, OUTPUT);
    digitalWrite(MODEM_PWR, LOW);

    // Force a clean power cycle so SIM interface re-initialises from cold.
    // If the modem is already on, the first pulse turns it OFF; if it's off,
    // it turns it ON — so we detect first, then pulse to the desired state.
    Serial.println("[PWR] checking modem state...");
    bool was_on = at("AT", 1500).indexOf("OK") >= 0;
    if (was_on) {
        Serial.println("[PWR] modem was ON — powering off...");
        digitalWrite(MODEM_PWR, HIGH); delay(1200); digitalWrite(MODEM_PWR, LOW);
        delay(5000);  // wait for clean shutdown
        Serial.println("[PWR] powering back on...");
    } else {
        Serial.println("[PWR] modem was OFF — powering on...");
    }
    digitalWrite(MODEM_PWR, HIGH); delay(1500); digitalWrite(MODEM_PWR, LOW);
    Serial.println("[PWR] waiting 15 s for modem boot...");
    for (int i = 15; i > 0; i--) {
        delay(1000);
        String r = at("AT", 500);
        Serial.printf("[PWR]   %2d s — %s\n", i, r.length() ? r.c_str() : "(no response)");
        if (r.indexOf("OK") >= 0) break;
    }

    if (at("AT", 2000).indexOf("OK") < 0) {
        Serial.println("[PWR] ERROR: modem not responding — check power supply and PWRKEY wiring");
        return;
    }
    Serial.println("[PWR] modem ready");

    Serial.println("\n=== SIM / Network Diagnostic ===\n");
    at("ATE0");  // echo off

    Serial.println("-- Modem --");
    print_at("Alive:",          "AT");
    print_at("IMEI:",           "AT+CGSN");
    print_at("Firmware:",       "AT+CGMR");

    Serial.println("\n-- SIM interface --");
    print_at("CPIN (PIN):",     "AT+CPIN?",        3000);
    print_at("CIMI (IMSI):",    "AT+CIMI",         3000);
    print_at("SIM det. mode:",  "AT+CSIMDET?");     // detection enabled/disabled
    print_at("SIM voltage:",    "AT+CCID",         3000); // ICCID (unique card ID)
    print_at("ICCID:",          "AT+CICCID",       3000);
    print_at("Card slot:",      "AT+SIMSLOT?");     // which slot is active

    // Try disabling SIM detection pin requirement (works on some SIM7600 variants
    // where the SIMDET pin is unconnected on the PCB)
    Serial.println("\n-- SIM detection override --");
    Serial.printf("CSIMDET=0,0:         %s\n", at("AT+CSIMDET=0,0").c_str());
    delay(500);
    Serial.printf("CSIMDET=1,0:         %s\n", at("AT+CSIMDET=1,0").c_str());
    delay(1000);
    print_at("CPIN after ovrd:",  "AT+CPIN?",      3000);
    print_at("CIMI after ovrd:",  "AT+CIMI",       3000);

    // Raw SIM access — if the card is electrically present this returns data
    Serial.println("\n-- Raw SIM probe --");
    print_at("SIM ATR (CSIM):",  "AT+CSIM=10,\"A0A40000023F00\"", 3000);

    Serial.println("\n-- RF / Network --");
    print_at("Signal (CSQ):",   "AT+CSQ");
    print_at("Registration:",   "AT+CREG?");
    print_at("GPRS reg:",       "AT+CGREG?");
    print_at("Operator:",       "AT+COPS?",        5000);
    print_at("Network mode:",   "AT+CNMP?");       // preferred mode (LTE/auto)

    Serial.println("\n-- SSL / CCH stack --");
    // Configure SSL context 0
    Serial.printf("CSSLCFG ignoretime:  %s\n", at("AT+CSSLCFG=\"ignorelocaltime\",0,1").c_str());
    Serial.printf("CSSLCFG sslver=3:    %s\n", at("AT+CSSLCFG=\"sslversion\",0,3").c_str());
    Serial.printf("CSSLCFG verify=0:    %s\n", at("AT+CSSLCFG=\"verify\",0,0").c_str());
    // Try starting the CCH SSL service
    Serial.printf("CCHSTART:            %s\n", at("AT+CCHSTART", 5000).c_str());
    delay(1000);
    // Try opening an SSL connection to HiveMQ (will fail but error code is informative)
    Serial.println("CCHOPEN (30s) ...");
    Serial.printf("CCHOPEN result:      %s\n",
        at("AT+CCHOPEN=0,\"35662fbd7a4646d9b8abb69de8a08244.s1.eu.hivemq.cloud\",8883,0", 30000).c_str());
    // Try CIPSSL (available on some SIM7600 firmwares)
    Serial.printf("CIPSSL=1:            %s\n", at("AT+CIPSSL=1").c_str());
    // TLS version list
    print_at("CSSLCFG?:",          "AT+CSSLCFG?");

    Serial.println("\n=== Done ===");
}

void loop() {}
