#pragma once
#include <Client.h>
#include <Arduino.h>

// SSL Client for SIM7600 using the AT+CCHOPEN channel stack.
// TinyGSM's SIM7600 driver only supports plain TCP (AT+CIPOPEN); this class
// uses the modem's dedicated SSL channel (AT+CCH*) which handles TLS natively.
// Shares Serial1 with TinyGSM — safe as long as callers are sequential.
class Sim7600SslClient : public Client {
public:
    Sim7600SslClient();

    int     connect(const char* host, uint16_t port) override;
    int     connect(IPAddress ip, uint16_t port) override { return 0; }
    size_t  write(uint8_t b) override { return write(&b, 1); }
    size_t  write(const uint8_t* buf, size_t size) override;
    int     available() override;
    int     read() override;
    int     read(uint8_t* buf, size_t size) override;
    int     peek() override { return (_buf_pos < _buf_len) ? _buf[_buf_pos] : -1; }
    void    flush() override {}
    void    stop() override;
    uint8_t connected() override;
    operator bool() override { return connected(); }

private:
    bool    _open;
    uint8_t _buf[512];
    size_t  _buf_len;
    size_t  _buf_pos;

    int fetchData();
};
