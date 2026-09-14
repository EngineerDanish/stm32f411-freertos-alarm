#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t tick_ms;
    uint16_t raw;
    float    temp_c;
} sensor_sample_t;

typedef enum {
    ALARM_NORMAL = 0,
    ALARM_ACTIVE
} alarm_state_t;

typedef struct {
    alarm_state_t   state;
    sensor_sample_t last;
    uint32_t        transitions;
    uint32_t        samples_seen;
} alarm_status_t;

#endif
