#include "app_init.h"
#include "app_config.h"
#include "app_types.h"
#include "app_log.h"
#include "sensor_task.h"
#include "alarm_task.h"
#include "report_task.h"

#include "main.h"

QueueHandle_t sensor_queue;
TaskHandle_t  h_sensor;
TaskHandle_t  h_alarm;
TaskHandle_t  h_report;

/* Everything is allocated here, before the scheduler starts. Nothing in this
   application allocates at runtime. */
void app_init(void)
{
    if (!app_log_init()) {
        Error_Handler();
    }

    sensor_queue = xQueueCreate(SENSOR_QUEUE_LEN, sizeof(sensor_sample_t));
    if (sensor_queue == NULL) {
        Error_Handler();
    }

    if (xTaskCreate(sensor_task, "sensor", STACK_SENSOR, NULL,
                    PRIO_SENSOR, &h_sensor) != pdPASS) {
        Error_Handler();
    }

    if (xTaskCreate(alarm_task, "alarm", STACK_ALARM, NULL,
                    PRIO_ALARM, &h_alarm) != pdPASS) {
        Error_Handler();
    }

    if (xTaskCreate(report_task, "report", STACK_REPORT, NULL,
                    PRIO_REPORT, &h_report) != pdPASS) {
        Error_Handler();
    }
}
