#ifndef ALARM_TASK_H
#define ALARM_TASK_H

#include "app_types.h"

void alarm_task(void *arg);
void alarm_get_status(alarm_status_t *out);

#endif
