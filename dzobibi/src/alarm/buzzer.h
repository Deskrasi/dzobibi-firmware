#pragma once
#include <stdint.h>

enum class BuzzerPattern : uint8_t {
    OFF = 0,
    ALARM,    // 3× 60 ms beeps + 300 ms gap, repeating
    TEST,     // single 150 ms beep then off
    CHIRP,    // single 50 ms chirp (low-bat warning etc.)
};

void buzzer_init();
void buzzer_set(BuzzerPattern p);
void buzzer_tick();   // must be called frequently from SafetyTask loop
bool buzzer_is_done();  // true once a one-shot pattern (TEST/CHIRP) finishes
