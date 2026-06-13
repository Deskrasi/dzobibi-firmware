#pragma once
#include <stdint.h>

enum class AlarmState : uint8_t {
    IDLE = 0,
    WARNING,
    ACTIVE,
    SILENCED,
    TEST,
};

struct AlarmInputs {
    float   co_ppm;
    int32_t voc_index;
    int32_t voc_prev_index;  // previous sample for rate-of-rise
    int32_t nox_index;
};

void  alarm_init();
// Called from SafetyTask. Returns true if state changed.
bool  alarm_update(const AlarmInputs& in);
void  alarm_silence();           // silence button or cmd:silence
void  alarm_test();              // cmd:test or long button press
void  alarm_tick();              // call every loop iteration (buzzer + hush timer)

AlarmState alarm_state();
bool  alarm_is_active();         // state == ACTIVE || state == TEST
