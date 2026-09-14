#include "report_task.h"
#include "alarm_task.h"
#include "sensor_task.h"
#include "app_config.h"
#include "app_init.h"
#include "app_log.h"

#include "FreeRTOS.h"
#include "task.h"

void report_task(void *arg)
{
    (void)arg;

    TickType_t next = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(REPORT_PERIOD_MS);

    for (;;) {
        alarm_status_t st;
        alarm_get_status(&st);

        app_log("[%lu] %s  temp=%d.%02u C  raw=%u  n=%lu  dropped=%lu",
                (unsigned long)st.last.tick_ms,
                (st.state == ALARM_ACTIVE) ? "ALARM " : "normal",
                (int)st.last.temp_c,
                (unsigned)((st.last.temp_c - (int)st.last.temp_c) * 100.0f),
                (unsigned)st.last.raw,
                (unsigned long)st.samples_seen,
                (unsigned long)sensor_dropped_count());

        app_log("      stack free: sensor=%u alarm=%u report=%u",
                (unsigned)uxTaskGetStackHighWaterMark(h_sensor),
                (unsigned)uxTaskGetStackHighWaterMark(h_alarm),
                (unsigned)uxTaskGetStackHighWaterMark(NULL));

        vTaskDelayUntil(&next, period);
    }
}
