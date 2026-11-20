#include "bringup_logic.h"

#include <stddef.h>

static void startup_enter_fault(bringup_startup_t *startup, const char *code,
                                const char *reason, uint64_t now_ms)
{
    startup->state = BRINGUP_STATE_FAULT;
    startup->fault_code = code;
    startup->fault_reason = reason;
    startup->fault_since_ms = now_ms;
    startup->last_fault_report_ms = now_ms;
    startup->fault_reported = false;
}

void bringup_startup_init(bringup_startup_t *startup, unsigned max_attempts)
{
    if (startup != NULL) {
        *startup = (bringup_startup_t){ .state = BRINGUP_STATE_INIT,
                                        .max_attempts = max_attempts };
    }
}

unsigned bringup_startup_begin_attempt(bringup_startup_t *startup)
{
    if (startup == NULL || startup->state != BRINGUP_STATE_INIT ||
        startup->attempts_used >= startup->max_attempts) {
        return 0U;
    }
    return ++startup->attempts_used;
}

void bringup_startup_camera_result(bringup_startup_t *startup, bool success,
                                   uint64_t now_ms)
{
    if (startup == NULL || startup->state != BRINGUP_STATE_INIT ||
        startup->attempts_used == 0U) {
        return;
    }
    if (success) {
        startup->state = BRINGUP_STATE_RUNNING;
    } else if (startup->attempts_used >= startup->max_attempts) {
        startup_enter_fault(startup, "RECOVERY_EXHAUSTED",
                            "camera_init_exhausted", now_ms);
    }
}

void bringup_startup_psram_failure(bringup_startup_t *startup,
                                   const char *code, const char *reason,
                                   uint64_t now_ms)
{
    if (startup != NULL && startup->state == BRINGUP_STATE_INIT) {
        startup_enter_fault(startup, code, reason, now_ms);
    }
}

bool bringup_startup_fault_due(bringup_startup_t *startup, uint64_t now_ms,
                               uint32_t period_ms)
{
    if (startup == NULL || startup->state != BRINGUP_STATE_FAULT ||
        now_ms < startup->fault_since_ms) {
        return false;
    }
    if (!startup->fault_reported ||
        (now_ms >= startup->last_fault_report_ms &&
         now_ms - startup->last_fault_report_ms >= period_ms)) {
        startup->last_fault_report_ms = now_ms;
        startup->fault_reported = true;
        return true;
    }
    return false;
}

void bringup_measurement_start(bringup_measurement_t *window, uint64_t now_ms,
                               uint16_t width, uint16_t height)
{
    if (window == NULL) {
        return;
    }
    *window = (bringup_measurement_t){
        .started_ms = now_ms,
        .width = width,
        .height = height,
    };
}

void bringup_measurement_record(bringup_measurement_t *window,
                                const bringup_frame_observation_t *frame)
{
    if (window == NULL || frame == NULL) {
        return;
    }
    if (frame->valid && frame->width == window->width &&
        frame->height == window->height) {
        ++window->valid;
    } else {
        ++window->invalid;
    }
}

bool bringup_measurement_due(const bringup_measurement_t *window,
                             uint64_t now_ms, uint32_t period_ms)
{
    return window != NULL && now_ms >= window->started_ms &&
           now_ms - window->started_ms >= period_ms;
}

bool bringup_measurement_finish(bringup_measurement_t *window,
                                uint64_t now_ms, bringup_measurement_t *report)
{
    if (window == NULL || report == NULL || now_ms <= window->started_ms ||
        window == report) {
        return false;
    }
    *report = *window;
    report->elapsed_ms = now_ms - window->started_ms;
    report->fps = (double)report->valid * 1000.0 / (double)report->elapsed_ms;
    bringup_measurement_start(window, now_ms, report->width, report->height);
    return true;
}

/* The transition bodies intentionally return NONE until the US3 red tests
 * define the boundary behavior. */
void bringup_recovery_init(bringup_recovery_t *recovery,
                           const bringup_recovery_config_t *config,
                           uint64_t now_ms)
{
    if (recovery == NULL || config == NULL) {
        return;
    }
    *recovery = (bringup_recovery_t){
        .phase = BRINGUP_RECOVERY_PHASE_RUNNING,
        .config = *config,
        .remaining = config->recovery_attempts,
        .last_valid_ms = now_ms,
        .running_since_ms = now_ms,
    };
}

bringup_recovery_action_t bringup_recovery_on_frame(bringup_recovery_t *recovery,
                                                    bool valid, uint64_t now_ms)
{
    (void)recovery;
    (void)valid;
    (void)now_ms;
    return BRINGUP_RECOVERY_ACTION_NONE;
}

bringup_recovery_action_t bringup_recovery_on_tick(bringup_recovery_t *recovery,
                                                   uint64_t now_ms)
{
    (void)recovery;
    (void)now_ms;
    return BRINGUP_RECOVERY_ACTION_NONE;
}

bringup_recovery_action_t bringup_recovery_on_worker_ready(bringup_recovery_t *recovery,
                                                           uint64_t now_ms)
{
    (void)recovery;
    (void)now_ms;
    return BRINGUP_RECOVERY_ACTION_NONE;
}

bringup_recovery_action_t bringup_recovery_on_worker_done(bringup_recovery_t *recovery,
                                                          bringup_worker_done_t done,
                                                          uint64_t now_ms)
{
    (void)recovery;
    (void)done;
    (void)now_ms;
    return BRINGUP_RECOVERY_ACTION_NONE;
}
