#ifndef APP_INIT_H
#define APP_INIT_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern QueueHandle_t sensor_queue;
extern TaskHandle_t  h_sensor;
extern TaskHandle_t  h_alarm;
extern TaskHandle_t  h_report;

/* Call once from main() before vTaskStartScheduler(). */
void app_init(void);

#endif
