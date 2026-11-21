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

/* US3 recovery interface. Time and configuration are supplied by the caller. */
typedef enum {
    BRINGUP_RECOVERY_PHASE_RUNNING,
    BRINGUP_RECOVERY_PHASE_STOPPING,
    BRINGUP_RECOVERY_PHASE_STARTING,
    BRINGUP_RECOVERY_PHASE_WAITING_FIRST_FRAME,
    BRINGUP_RECOVERY_PHASE_FAULT,
} bringup_recovery_phase_t;

typedef enum {
    BRINGUP_WORKER_DONE_STOPPED,
    BRINGUP_WORKER_DONE_INIT_FAILED,
} bringup_worker_done_t;

typedef enum {
    BRINGUP_RECOVERY_ACTION_NONE = 0,
    BRINGUP_RECOVERY_ACTION_REQUEST_STOP = 1U << 0,
    BRINGUP_RECOVERY_ACTION_START_WORKER = 1U << 1,
    BRINGUP_RECOVERY_ACTION_REPORT_WAIT = 1U << 2,
    BRINGUP_RECOVERY_ACTION_SUCCESS = 1U << 3,
    BRINGUP_RECOVERY_ACTION_FAILURE = 1U << 4,
    BRINGUP_RECOVERY_ACTION_BUDGET_RESET = 1U << 5,
    BRINGUP_RECOVERY_ACTION_FAULT = 1U << 6,
} bringup_recovery_action_t;

typedef struct {
    unsigned recovery_attempts;
    unsigned invalid_frame_limit;
    uint32_t frame_loss_timeout_ms;
    uint32_t first_frame_timeout_ms;
    uint32_t stable_reset_ms;
    uint32_t stop_wait_limit_ms;
    uint32_t recovery_wait_period_ms;
} bringup_recovery_config_t;

typedef struct {
    bringup_recovery_phase_t phase;
    bringup_recovery_config_t config;
    unsigned remaining;
    unsigned consecutive_invalid;
    uint64_t last_valid_ms;
    uint64_t running_since_ms;
    uint64_t stable_since_ms;
    uint64_t stop_requested_ms;
    uint64_t worker_ready_ms;
    uint64_t last_wait_report_ms;
    bool stable_timer_active;
    bool wait_reported;
    const char *fault_code;
    const char *last_cause;
} bringup_recovery_t;

void bringup_recovery_init(bringup_recovery_t *recovery,
                           const bringup_recovery_config_t *config,
                           uint64_t now_ms);
bringup_recovery_action_t bringup_recovery_on_frame(bringup_recovery_t *recovery,
                                                    bool valid, uint64_t now_ms);
bringup_recovery_action_t bringup_recovery_on_tick(bringup_recovery_t *recovery,
                                                   uint64_t now_ms);
bringup_recovery_action_t bringup_recovery_on_worker_ready(bringup_recovery_t *recovery,
                                                           uint64_t now_ms);
bringup_recovery_action_t bringup_recovery_on_worker_done(bringup_recovery_t *recovery,
                                                          bringup_worker_done_t done,
                                                          uint64_t now_ms);

#endif
