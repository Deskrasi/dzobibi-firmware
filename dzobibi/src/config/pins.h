#pragma once

// Buzzer — HIGH = on (→ Q5 N-FET → LS1, self-driving ~2.4 kHz)
#define PIN_BUZZER          9

// Battery ADC — reads ½ of BAT+ through 10k/10k divider
#define PIN_BATT_SENSE      4   // ADC1_CH3
#define PIN_BAT_MEASURE     17  // HIGH = enable divider via Q4

// RGB status LED (WS2812B) — best-effort Rev A (VDD on BAT+, marginal logic)
#define PIN_RGB_LED         10

// CO sensor (TGS5141 via MCP6042 TIA)
#define PIN_CO_SENSOR       1   // ADC1_CH0

// SGP41 VOC/NOx (I²C addr 0x59)
#define PIN_SDA             11
#define PIN_SCL             12

// SIM7600G-H cellular modem (via TXS0108 level shifter)
#define PIN_MODEM_TX        43  // ESP32 TX → SIM7600 RXD
#define PIN_MODEM_RX        44  // ESP32 RX ← SIM7600 TXD
#define PIN_MODEM_PWRKEY    35  // HIGH = pull PWRKEY low (~1 s pulse to toggle)

// Silence / test button (BOOT button, active-low, external pull-up)
// Note: strapping pin — never check during reset sequence
#define PIN_SILENCE_BTN     0
