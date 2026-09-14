#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Task priorities. Higher number = higher priority in FreeRTOS. */
#define PRIO_SENSOR             3
#define PRIO_ALARM              2
#define PRIO_REPORT             1

/* Stack sizes in words, not bytes. */
#define STACK_SENSOR            192
#define STACK_ALARM             192
#define STACK_REPORT            320   /* snprintf needs the extra room */

/* Timing */
#define SENSOR_PERIOD_MS        100
#define REPORT_PERIOD_MS        2000

/* Queue between sensor and alarm task */
#define SENSOR_QUEUE_LEN        4

/* Threshold with hysteresis, in degrees C.
   Trip above RISE, clear only below FALL. */
#define ALARM_RISE_C            40.0f
#define ALARM_FALL_C            37.0f

/* Consecutive samples required before a state change is accepted. */
#define ALARM_CONFIRM_COUNT     3

/* Blocking time for a UART transmit, in ms. */
#define LOG_UART_TIMEOUT_MS     100

/* Board mapping. Nucleo-F411RE. */
#define ALARM_LED_PORT          GPIOA
#define ALARM_LED_PIN           GPIO_PIN_5
#define ALARM_LED_ACTIVE_HIGH   1

#endif
