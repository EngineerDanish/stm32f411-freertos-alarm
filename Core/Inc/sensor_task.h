#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <stdint.h>

void     sensor_task(void *arg);
uint32_t sensor_dropped_count(void);

#endif
