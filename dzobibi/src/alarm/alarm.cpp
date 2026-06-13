#include "alarm.h"
#include "buzzer.h"
#include "../config/constants.h"
#include <Arduino.h>

static AlarmState s_state           = AlarmState::IDLE;
static int        s_confirm_count   = 0;
static int        s_clear_count     = 0;
static uint32_t   s_hush_start      = 0;
static uint32_t   s_test_start      = 0;
static uint32_t   s_alarm_start     = 0;

static bool is_dangerous(const AlarmInputs& in) {
    if (in.co_ppm >= CO_CRITICAL_PPM) return true;
    if (in.co_ppm >= CO_ALARM_PPM)    return true;
    if (in.voc_index >= VOC_ALARM_INDEX) return true;
    int32_t rise = in.voc_index - in.voc_prev_index;
    if (rise >= VOC_RISE_DELTA)        return true;
    return false;
}

static bool is_critical(const AlarmInputs& in) {
    return in.co_ppm >= CO_CRITICAL_PPM;
}

static bool is_warning(const AlarmInputs& in) {
    return in.co_ppm >= CO_WARNING_PPM;
}

void alarm_init() {
    buzzer_init();
    s_state = AlarmState::IDLE;
}

bool alarm_update(const AlarmInputs& in) {
    AlarmState prev = s_state;

    switch (s_state) {
        case AlarmState::IDLE:
        case AlarmState::WARNING: {
            if (is_dangerous(in) || is_critical(in)) {
                if (is_critical(in)) {
                    // No debounce for critical CO — immediate
                    s_state = AlarmState::ACTIVE;
                    s_alarm_start = millis();
                    s_confirm_count = 0;
                } else {
                    s_confirm_count++;
                    if (s_confirm_count >= ALARM_DEBOUNCE) {
                        s_state = AlarmState::ACTIVE;
                        s_alarm_start = millis();
                        s_confirm_count = 0;
                    } else if (is_warning(in)) {
                        s_state = AlarmState::WARNING;
                    }
                }
                s_clear_count = 0;
            } else {
                s_confirm_count = 0;
                if (s_state == AlarmState::WARNING) {
                    s_clear_count++;
                    if (s_clear_count >= ALARM_DEBOUNCE) {
                        s_state = AlarmState::IDLE;
                        s_clear_count = 0;
                    }
                }
            }
            break;
        }

        case AlarmState::ACTIVE:
            // Stay latched while dangerous; need 30 s of clear air to reset
            if (!is_dangerous(in)) {
                if ((millis() - s_alarm_start) > 30000UL) {
                    s_confirm_count = 0;
                    s_state = AlarmState::IDLE;
                }
            } else {
                s_alarm_start = millis();  // reset clear timer
            }
            break;

        case AlarmState::SILENCED:
            // Worsening to critical overrides hush
            if (is_critical(in)) {
                s_state = AlarmState::ACTIVE;
                s_alarm_start = millis();
                break;
            }
            // Hush expired while still dangerous
            if ((millis() - s_hush_start) >= ALARM_HUSH_MS) {
                if (is_dangerous(in)) {
                    s_state = AlarmState::ACTIVE;
                    s_alarm_start = millis();
                } else {
                    s_state = AlarmState::IDLE;
                }
            }
            // Cleared before hush expires
            if (!is_dangerous(in) && !is_warning(in)) {
                s_state = AlarmState::IDLE;
            }
            break;

        case AlarmState::TEST:
            if (buzzer_is_done() || (millis() - s_test_start) > ALARM_TEST_MS) {
                s_state = AlarmState::IDLE;
                buzzer_set(BuzzerPattern::OFF);
            }
            break;
    }

    // Drive buzzer from state
    if (s_state == AlarmState::ACTIVE && prev != AlarmState::ACTIVE) {
        buzzer_set(BuzzerPattern::ALARM);
    } else if (s_state != AlarmState::ACTIVE && s_state != AlarmState::TEST) {
        if (prev == AlarmState::ACTIVE || prev == AlarmState::TEST) {
            buzzer_set(BuzzerPattern::OFF);
        }
    }

    return s_state != prev;
}

void alarm_silence() {
    if (s_state == AlarmState::ACTIVE) {
        s_state = AlarmState::SILENCED;
        s_hush_start = millis();
        buzzer_set(BuzzerPattern::OFF);
    }
}

void alarm_test() {
    s_state = AlarmState::TEST;
    s_test_start = millis();
    buzzer_set(BuzzerPattern::TEST);
}

void alarm_tick() {
    buzzer_tick();
}

AlarmState alarm_state()    { return s_state; }
bool alarm_is_active()      { return s_state == AlarmState::ACTIVE || s_state == AlarmState::TEST; }
