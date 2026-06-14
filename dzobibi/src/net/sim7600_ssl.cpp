#include "sim7600_ssl.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static constexpr size_t   RX_BUF_SZ   = 512;
static constexpr uint32_t T_SHORT      = 5000;
static constexpr uint32_t T_CONNECT    = 30000;
static constexpr uint32_t T_SEND       = 10000;

static bool s_cch_started = false;

// ── Serial1 helpers ───────────────────────────────────────────────────────────

// Accumulate bytes from Serial1 until `target` appears or timeout.
// Returns true if target was found; *out contains everything read.
static bool wait_for(const char* target, uint32_t timeout_ms, String* out = nullptr) {
    uint32_t t = millis();
    String buf;
    while (millis() - t < timeout_ms) {
        while (Serial1.available()) {
            buf += (char)Serial1.read();
            if (buf.indexOf(target) >= 0) {
                if (out) *out = buf;
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    if (out) *out = buf;
    return false;
}

// Read exactly n raw bytes with timeout.
static size_t read_exact(uint8_t* dst, size_t n, uint32_t timeout_ms) {
    uint32_t t = millis();
    size_t got = 0;
    while (got < n && millis() - t < timeout_ms) {
        if (Serial1.available()) {
            dst[got++] = (uint8_t)Serial1.read();
        } else {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
    return got;
}

// Drain leftover modem bytes (e.g., trailing OK after a response).
static void drain(uint32_t wait_ms = 50) {
    vTaskDelay(pdMS_TO_TICKS(wait_ms));
    while (Serial1.available()) Serial1.read();
}

// ── Sim7600SslClient ──────────────────────────────────────────────────────────

Sim7600SslClient::Sim7600SslClient()
    : _open(false), _buf_len(0), _buf_pos(0) {}

int Sim7600SslClient::connect(const char* host, uint16_t port) {
    _open    = false;
    _buf_len = _buf_pos = 0;

    // AT+CCHSTART initialises the SSL service (idempotent across connections,
    // but modem may return ERROR if already started — treat both OK and ERROR
    // as acceptable).
    if (!s_cch_started) {
        drain(100);
        Serial1.println("AT+CCHSTART");
        String r;
        wait_for("OK", T_SHORT, &r);  // "ERROR" if already running — fine
        s_cch_started = true;
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    // Open session 0 in command mode (access_mode=0).
    // The SIM7600 uses SSL context 0 by default; cellular_init() configures it
    // to ignore local time so cert validation doesn't fail on clock skew.
    char cmd[160];
    snprintf(cmd, sizeof(cmd), "AT+CCHOPEN=0,\"%s\",%u,0", host, port);
    drain(50);
    Serial1.println(cmd);
    Serial.printf("[SSL] opening %s:%u ...\n", host, port);

    // Wait for async: +CCHOPEN: <ssid>,<err>
    String resp;
    if (!wait_for("+CCHOPEN:", T_CONNECT, &resp)) {
        Serial.printf("[SSL] connect timeout (resp: %s)\n", resp.c_str());
        s_cch_started = false;  // force CCHSTART re-init on next attempt
        return 0;
    }

    // Parse error code — format: "+CCHOPEN: 0,0" or "+CCHOPEN:0,0"
    int idx   = resp.lastIndexOf("+CCHOPEN:");
    String after = resp.substring(idx + 9);  // " 0,0" or "0,0"
    after.trim();
    int comma = after.indexOf(',');
    int err   = (comma >= 0) ? after.substring(comma + 1).toInt() : -1;

    if (err != 0) {
        Serial.printf("[SSL] +CCHOPEN err=%d\n", err);
        return 0;
    }

    Serial.println("[SSL] TLS connected");
    _open = true;
    return 1;
}

size_t Sim7600SslClient::write(const uint8_t* buf, size_t size) {
    if (!_open || size == 0) return 0;

    drain(20);
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CCHSEND=0,%u", (unsigned)size);
    Serial1.println(cmd);

    // Modem sends '>' prompt when ready to receive payload
    if (!wait_for(">", T_SHORT)) {
        Serial.println("[SSL] write: no > prompt");
        _open = false;
        return 0;
    }

    Serial1.write(buf, size);
    Serial1.flush();

    // Wait for async send acknowledgement (non-fatal if it's slow)
    String ack;
    wait_for("+CCHSEND:", T_SEND, &ack);
    if (ack.indexOf("+CCHSEND:") >= 0) {
        // Check for non-zero error code
        int cidx = ack.lastIndexOf('+');
        String a2 = ack.substring(cidx + 9);
        int comma2 = a2.indexOf(',');
        int serr = (comma2 >= 0) ? a2.substring(comma2 + 1).toInt() : 0;
        if (serr != 0) {
            Serial.printf("[SSL] +CCHSEND err=%d\n", serr);
            _open = false;
            return 0;
        }
    }

    return size;
}

int Sim7600SslClient::fetchData() {
    _buf_len = _buf_pos = 0;

    drain(20);
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CCHRECV=0,%u", (unsigned)RX_BUF_SZ);
    Serial1.println(cmd);

    // Response: "+CCHRECV: <ssid>,<len>\r\n<data>\r\nOK"  or "ERROR"
    String hdr;
    if (!wait_for("+CCHRECV:", T_SHORT, &hdr)) {
        // ERROR or timeout — check for connection-closed indicator
        if (hdr.indexOf("CLOSE EVENT") >= 0 || hdr.indexOf("ERROR") >= 0) {
            _open = false;
        }
        drain(30);
        return 0;
    }

    // Read remainder of the header line: " <ssid>,<datalen>\r\n"
    String len_line;
    wait_for("\n", T_SHORT, &len_line);

    // Take the number after the last comma (handles both "0,64" and "DATA,64")
    int comma = len_line.lastIndexOf(',');
    if (comma < 0) { drain(50); return 0; }
    int len = len_line.substring(comma + 1).toInt();
    if (len <= 0) { drain(50); return 0; }
    if (len > (int)RX_BUF_SZ) len = (int)RX_BUF_SZ;

    size_t got = read_exact(_buf, (size_t)len, T_SHORT);
    _buf_len = got;
    drain(50);  // eat trailing \r\nOK\r\n
    return (int)got;
}

int Sim7600SslClient::available() {
    if (_buf_pos < _buf_len) return (int)(_buf_len - _buf_pos);
    if (!_open) return 0;
    return fetchData();
}

int Sim7600SslClient::read() {
    if (_buf_pos >= _buf_len && fetchData() == 0) return -1;
    return (int)_buf[_buf_pos++];
}

int Sim7600SslClient::read(uint8_t* buf, size_t size) {
    if (_buf_pos >= _buf_len && fetchData() == 0) return 0;
    size_t avail = _buf_len - _buf_pos;
    size_t n = (size < avail) ? size : avail;
    memcpy(buf, _buf + _buf_pos, n);
    _buf_pos += n;
    return (int)n;
}

void Sim7600SslClient::stop() {
    if (_open) {
        drain(20);
        Serial1.println("AT+CCHCLOSE=0");
        wait_for("CCHCLOSE", T_SHORT);
        _open = false;
    }
    _buf_len = _buf_pos = 0;
}

uint8_t Sim7600SslClient::connected() {
    if (!_open) return 0;
    // Opportunistically drain the stream and check for a close event that
    // TinyGSM may not have consumed (best-effort; fetchData() also checks).
    while (Serial1.available()) {
        static String _peek_buf;
        _peek_buf += (char)Serial1.read();
        if (_peek_buf.indexOf("CLOSE EVENT") >= 0) {
            _open = false;
            _peek_buf = "";
            return 0;
        }
        if (_peek_buf.length() > 64) _peek_buf = _peek_buf.substring(32);
    }
    return 1;
}
