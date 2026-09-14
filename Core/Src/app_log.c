#include "app_log.h"
#include "app_config.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "main.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;

static SemaphoreHandle_t uart_mutex;
static char line[128];

bool app_log_init(void)
{
    uart_mutex = xSemaphoreCreateMutex();
    return uart_mutex != NULL;
}

/* Two tasks write to the same UART: the alarm task on state changes and the
   report task periodically. The mutex serialises them. The line buffer is
   static and shared, so it is only touched while the mutex is held. */
void app_log(const char *fmt, ...)
{
    if (uart_mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(uart_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(line, sizeof(line) - 2, fmt, args);
    va_end(args);

    if (n > 0) {
        if ((size_t)n > sizeof(line) - 3) {
            n = sizeof(line) - 3;
        }
        line[n]     = '\r';
        line[n + 1] = '\n';
        HAL_UART_Transmit(&huart2, (uint8_t *)line, n + 2,
                          LOG_UART_TIMEOUT_MS);
    }

    xSemaphoreGive(uart_mutex);
}
