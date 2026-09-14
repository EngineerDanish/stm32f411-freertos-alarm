#include "alarm_task.h"
#include "app_config.h"
#include "app_init.h"
#include "app_log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main.h"

static alarm_status_t status;

static void drive_output(alarm_state_t state)
{
    GPIO_PinState level;

#if ALARM_LED_ACTIVE_HIGH
    level = (state == ALARM_ACTIVE) ? GPIO_PIN_SET : GPIO_PIN_RESET;
#else
    level = (state == ALARM_ACTIVE) ? GPIO_PIN_RESET : GPIO_PIN_SET;
#endif

    HAL_GPIO_WritePin(ALARM_LED_PORT, ALARM_LED_PIN, level);
}

void alarm_task(void *arg)
{
    (void)arg;

    sensor_sample_t s;
    uint8_t above = 0;
    uint8_t below = 0;

    status.state = ALARM_NORMAL;
    drive_output(ALARM_NORMAL);

    for (;;) {
        if (xQueueReceive(sensor_queue, &s, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        taskENTER_CRITICAL();
        status.last = s;
        status.samples_seen++;
        taskEXIT_CRITICAL();

        /* Separate rise and fall thresholds stop the output chattering when
           the reading sits on the boundary. The confirmation counters stop a
           single noisy conversion from tripping it. */
        if (s.temp_c >= ALARM_RISE_C) {
            below = 0;
            if (above < ALARM_CONFIRM_COUNT) {
                above++;
            }
        } else if (s.temp_c <= ALARM_FALL_C) {
            above = 0;
            if (below < ALARM_CONFIRM_COUNT) {
                below++;
            }
        } else {
            above = 0;
            below = 0;
        }

        alarm_state_t next = status.state;

        if (status.state == ALARM_NORMAL && above >= ALARM_CONFIRM_COUNT) {
            next = ALARM_ACTIVE;
        } else if (status.state == ALARM_ACTIVE && below >= ALARM_CONFIRM_COUNT) {
            next = ALARM_NORMAL;
        }

        if (next != status.state) {
            /* Output first, log second. The physical output must not wait on
               a UART that another task may be holding. */
            drive_output(next);

            taskENTER_CRITICAL();
            status.state = next;
            status.transitions++;
            taskEXIT_CRITICAL();

            above = 0;
            below = 0;

            app_log("[%lu] ALARM %s at %d.%02u C",
                    (unsigned long)s.tick_ms,
                    (next == ALARM_ACTIVE) ? "SET" : "CLEAR",
                    (int)s.temp_c,
                    (unsigned)((s.temp_c - (int)s.temp_c) * 100.0f));
        }
    }
}

void alarm_get_status(alarm_status_t *out)
{
    taskENTER_CRITICAL();
    *out = status;
    taskEXIT_CRITICAL();
}
