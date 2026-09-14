#include "sensor_task.h"
#include "app_config.h"
#include "app_types.h"
#include "app_init.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

/* STM32F411 has no factory temperature calibration values, so the datasheet
   typical figures are used. Accuracy is roughly +/-3 C. Good enough to drive
   a threshold demo, not good enough to call it a measurement. */
#define ADC_VREF_MV     3300.0f
#define ADC_FULL_SCALE  4095.0f
#define TS_V25_MV       760.0f
#define TS_SLOPE_MV_C   2.5f

static volatile uint32_t dropped;

static bool read_raw(uint16_t *out)
{
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return false;
    }
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return false;
    }
    *out = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return true;
}

static float raw_to_celsius(uint16_t raw)
{
    float mv = ((float)raw * ADC_VREF_MV) / ADC_FULL_SCALE;
    return ((mv - TS_V25_MV) / TS_SLOPE_MV_C) + 25.0f;
}

void sensor_task(void *arg)
{
    (void)arg;

    TickType_t next = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(SENSOR_PERIOD_MS);

    for (;;) {
        sensor_sample_t s;
        uint16_t raw;

        if (read_raw(&raw)) {
            s.tick_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
            s.raw     = raw;
            s.temp_c  = raw_to_celsius(raw);

            /* Zero timeout on purpose. If the alarm task is behind, the
               sampling cadence still matters more than this one reading, so
               the sample is dropped and counted rather than blocking here. */
            if (xQueueSend(sensor_queue, &s, 0) != pdTRUE) {
                dropped++;
            }
        }

        vTaskDelayUntil(&next, period);
    }
}

uint32_t sensor_dropped_count(void)
{
    return dropped;
}
