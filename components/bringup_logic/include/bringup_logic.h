#ifndef BRINGUP_LOGIC_H
#define BRINGUP_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BRINGUP_STATE_INIT,
    BRINGUP_STATE_RUNNING,
    BRINGUP_STATE_RECOVERING,
    BRINGUP_STATE_FAULT,
} bringup_state_t;

typedef struct {
    uint64_t at_ms;
    bool valid;
    uint16_t width;
    uint16_t height;
} bringup_frame_observation_t;

typedef struct {
    uint64_t started_ms;
    uint64_t elapsed_ms;
    uint32_t valid;
    uint32_t invalid;
    uint16_t width;
    uint16_t height;
    double fps;
} bringup_measurement_t;

void bringup_measurement_start(bringup_measurement_t *window, uint64_t now_ms,
                               uint16_t width, uint16_t height);
void bringup_measurement_record(bringup_measurement_t *window,
                                const bringup_frame_observation_t *frame);
bool bringup_measurement_due(const bringup_measurement_t *window,
                             uint64_t now_ms, uint32_t period_ms);
bool bringup_measurement_finish(bringup_measurement_t *window,
                                uint64_t now_ms, bringup_measurement_t *report);

#endif
