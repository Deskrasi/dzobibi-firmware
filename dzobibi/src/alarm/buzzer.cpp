#include "buzzer.h"
#include "../config/pins.h"
#include <Arduino.h>

struct Step { bool on; uint32_t ms; };

// Alarm: 3× 60 ms on / 60 ms off, then 300 ms silence, repeat
static const Step STEPS_ALARM[] = {
    {true,60},{false,60},{true,60},{false,60},{true,60},{false,300},
};
// Test: single 150 ms beep
static const Step STEPS_TEST[] = { {true,150},{false,0} };
// Chirp: single 50 ms blip
static const Step STEPS_CHIRP[] = { {true,50},{false,0} };

static BuzzerPattern s_pattern   = BuzzerPattern::OFF;
static int           s_step      = 0;
static uint32_t      s_step_start= 0;
static bool          s_done      = false;

static const Step*   s_steps     = nullptr;
static int           s_step_count= 0;
static bool          s_loop      = false;

static void start_pattern(const Step* steps, int count, bool loop_it) {
    s_steps      = steps;
    s_step_count = count;
    s_step       = 0;
    s_step_start = millis();
    s_loop       = loop_it;
    s_done       = false;
    digitalWrite(PIN_BUZZER, steps[0].on ? HIGH : LOW);
}

void buzzer_init() {
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
}

void buzzer_set(BuzzerPattern p) {
    if (p == s_pattern) return;
    s_pattern = p;
    switch (p) {
        case BuzzerPattern::OFF:
            digitalWrite(PIN_BUZZER, LOW);
            s_steps = nullptr; s_done = true; break;
        case BuzzerPattern::ALARM:
            start_pattern(STEPS_ALARM, 6, true);  break;
        case BuzzerPattern::TEST:
            start_pattern(STEPS_TEST,  2, false); break;
        case BuzzerPattern::CHIRP:
            start_pattern(STEPS_CHIRP, 2, false); break;
    }
}

void buzzer_tick() {
    if (!s_steps || s_done) return;
    uint32_t elapsed = millis() - s_step_start;
    if (elapsed < s_steps[s_step].ms) return;

    s_step++;
    if (s_step >= s_step_count) {
        if (s_loop) {
            s_step = 0;
        } else {
            digitalWrite(PIN_BUZZER, LOW);
            s_done   = true;
            s_steps  = nullptr;
            s_pattern = BuzzerPattern::OFF;
            return;
        }
    }
    s_step_start = millis();
    digitalWrite(PIN_BUZZER, s_steps[s_step].on ? HIGH : LOW);
}

bool buzzer_is_done() { return s_done; }
