#pragma once

// Capture zero reference in clean air. Call once after system stable.
void  co_sensor_init();
void  co_sensor_zero();
float co_sensor_read_ppm();   // -1.0 on sensor fault
