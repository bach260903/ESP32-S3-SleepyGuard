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
    bringup_state_t state;
    unsigned attempts_used;
    unsigned max_attempts;
    const char *fault_code;
    const char *fault_reason;
    uint64_t fault_since_ms;
    uint64_t last_fault_report_ms;
    bool fault_reported;
} bringup_startup_t;

void bringup_startup_init(bringup_startup_t *startup, unsigned max_attempts);
unsigned bringup_startup_begin_attempt(bringup_startup_t *startup);
void bringup_startup_camera_result(bringup_startup_t *startup, bool success,
                                   uint64_t now_ms);
void bringup_startup_psram_failure(bringup_startup_t *startup,
                                   const char *code, const char *reason,
                                   uint64_t now_ms);
bool bringup_startup_fault_due(bringup_startup_t *startup, uint64_t now_ms,
                               uint32_t period_ms);

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
