#include "topics.h"
#include <Arduino.h>

char TOPIC_STATUS[80];
char TOPIC_TELEMETRY[80];
char TOPIC_ALARM[80];
char TOPIC_CMD[80];
char TOPIC_ACK[80];

void topics_init(const char* device_id) {
    snprintf(TOPIC_STATUS,    sizeof(TOPIC_STATUS),    "dzobibi/%s/status",    device_id);
    snprintf(TOPIC_TELEMETRY, sizeof(TOPIC_TELEMETRY), "dzobibi/%s/telemetry", device_id);
    snprintf(TOPIC_ALARM,     sizeof(TOPIC_ALARM),     "dzobibi/%s/alarm",     device_id);
    snprintf(TOPIC_CMD,       sizeof(TOPIC_CMD),       "dzobibi/%s/cmd",       device_id);
    snprintf(TOPIC_ACK,       sizeof(TOPIC_ACK),       "dzobibi/%s/ack",       device_id);
}
