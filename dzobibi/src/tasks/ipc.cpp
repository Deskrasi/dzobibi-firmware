#include "ipc.h"
#include "../config/constants.h"
#include <Arduino.h>

QueueHandle_t xAlarmEventQueue = nullptr;
QueueHandle_t xCmdQueue        = nullptr;
SharedState   g_state          = {};

void ipc_init() {
    xAlarmEventQueue = xQueueCreate(QUEUE_ALARM_DEPTH, sizeof(AlarmEvent));
    xCmdQueue        = xQueueCreate(QUEUE_CMD_DEPTH,   sizeof(Command));
    g_state.mutex    = xSemaphoreCreateMutex();
    g_state.alarm_state = AlarmState::IDLE;
    configASSERT(xAlarmEventQueue);
    configASSERT(xCmdQueue);
    configASSERT(g_state.mutex);
}

uint32_t ipc_now() {
    if (g_state.boot_epoch == 0) return millis() / 1000;
    return g_state.boot_epoch + (millis() - g_state.boot_millis) / 1000;
}
