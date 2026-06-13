#pragma once

// Populated once by topics_init(); then read-only for the lifetime of the device.
extern char TOPIC_STATUS[80];
extern char TOPIC_TELEMETRY[80];
extern char TOPIC_ALARM[80];
extern char TOPIC_CMD[80];
extern char TOPIC_ACK[80];

void topics_init(const char* device_id);
