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

static bringup_recovery_action_t recovery_enter_stopping(bringup_recovery_t *recovery,
                                                         uint64_t now_ms)
{
    recovery->phase = BRINGUP_RECOVERY_PHASE_STOPPING;
    recovery->stop_requested_ms = now_ms;
    recovery->last_wait_report_ms = now_ms;
    recovery->wait_reported = true;
    recovery->stable_timer_active = false;
    return BRINGUP_RECOVERY_ACTION_REQUEST_STOP |
           BRINGUP_RECOVERY_ACTION_REPORT_WAIT;
}

static bringup_recovery_action_t recovery_enter_fault(bringup_recovery_t *recovery,
                                                      const char *code)
{
    recovery->phase = BRINGUP_RECOVERY_PHASE_FAULT;
    recovery->fault_code = code;
    return BRINGUP_RECOVERY_ACTION_FAULT;
}

static bringup_recovery_action_t recovery_start_next(bringup_recovery_t *recovery)
{
    if (recovery->remaining == 0U) {
        return recovery_enter_fault(recovery, "RECOVERY_EXHAUSTED");
    }
    --recovery->remaining;
    recovery->phase = BRINGUP_RECOVERY_PHASE_STARTING;
    return BRINGUP_RECOVERY_ACTION_START_WORKER;
}

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
    if (recovery == NULL) {
        return BRINGUP_RECOVERY_ACTION_NONE;
    }
    if (recovery->phase == BRINGUP_RECOVERY_PHASE_WAITING_FIRST_FRAME) {
        if (now_ms < recovery->worker_ready_ms ||
            now_ms - recovery->worker_ready_ms >= recovery->config.first_frame_timeout_ms) {
            return BRINGUP_RECOVERY_ACTION_FAILURE |
                   recovery_enter_stopping(recovery, now_ms);
        }
        if (valid) {
            recovery->phase = BRINGUP_RECOVERY_PHASE_RUNNING;
            recovery->running_since_ms = now_ms;
            recovery->last_valid_ms = now_ms;
            recovery->stable_since_ms = now_ms;
            recovery->stable_timer_active = true;
            recovery->consecutive_invalid = 0U;
            return BRINGUP_RECOVERY_ACTION_SUCCESS;
        }
        if (++recovery->consecutive_invalid >= recovery->config.invalid_frame_limit) {
            return BRINGUP_RECOVERY_ACTION_FAILURE |
                   recovery_enter_stopping(recovery, now_ms);
        }
    } else if (recovery->phase == BRINGUP_RECOVERY_PHASE_RUNNING) {
        if (now_ms < recovery->last_valid_ms ||
            now_ms - recovery->last_valid_ms >= recovery->config.frame_loss_timeout_ms) {
            return recovery_enter_stopping(recovery, now_ms);
        }
        if (valid) {
            recovery->last_valid_ms = now_ms;
            recovery->consecutive_invalid = 0U;
        } else if (++recovery->consecutive_invalid >= recovery->config.invalid_frame_limit) {
            return recovery_enter_stopping(recovery, now_ms);
        }
    }
    return BRINGUP_RECOVERY_ACTION_NONE;
}

bringup_recovery_action_t bringup_recovery_on_tick(bringup_recovery_t *recovery,
                                                   uint64_t now_ms)
{
    if (recovery == NULL) {
        return BRINGUP_RECOVERY_ACTION_NONE;
    }
    if (recovery->phase == BRINGUP_RECOVERY_PHASE_RUNNING) {
        if (now_ms >= recovery->last_valid_ms &&
            now_ms - recovery->last_valid_ms >= recovery->config.frame_loss_timeout_ms) {
            return recovery_enter_stopping(recovery, now_ms);
        }
        if (recovery->stable_timer_active && now_ms >= recovery->stable_since_ms &&
            now_ms - recovery->stable_since_ms >= recovery->config.stable_reset_ms) {
            recovery->remaining = recovery->config.recovery_attempts;
            recovery->stable_timer_active = false;
            return BRINGUP_RECOVERY_ACTION_BUDGET_RESET;
        }
    } else if (recovery->phase == BRINGUP_RECOVERY_PHASE_WAITING_FIRST_FRAME) {
        if (now_ms >= recovery->worker_ready_ms &&
            now_ms - recovery->worker_ready_ms >= recovery->config.first_frame_timeout_ms) {
            return BRINGUP_RECOVERY_ACTION_FAILURE |
                   recovery_enter_stopping(recovery, now_ms);
        }
    } else if (recovery->phase == BRINGUP_RECOVERY_PHASE_STOPPING) {
        if (now_ms >= recovery->stop_requested_ms &&
            now_ms - recovery->stop_requested_ms >= recovery->config.stop_wait_limit_ms) {
            return recovery_enter_fault(recovery, "WORKER_STOP_TIMEOUT");
        }
        if (now_ms >= recovery->last_wait_report_ms &&
            now_ms - recovery->last_wait_report_ms >= recovery->config.recovery_wait_period_ms) {
            recovery->last_wait_report_ms = now_ms;
            return BRINGUP_RECOVERY_ACTION_REPORT_WAIT;
        }
    }
    return BRINGUP_RECOVERY_ACTION_NONE;
}

bringup_recovery_action_t bringup_recovery_on_worker_ready(bringup_recovery_t *recovery,
                                                           uint64_t now_ms)
{
    if (recovery != NULL && recovery->phase == BRINGUP_RECOVERY_PHASE_STARTING) {
        recovery->phase = BRINGUP_RECOVERY_PHASE_WAITING_FIRST_FRAME;
        recovery->worker_ready_ms = now_ms;
        recovery->consecutive_invalid = 0U;
    }
    return BRINGUP_RECOVERY_ACTION_NONE;
}

bringup_recovery_action_t bringup_recovery_on_worker_done(bringup_recovery_t *recovery,
                                                          bringup_worker_done_t done,
                                                          uint64_t now_ms)
{
    (void)now_ms;
    if (recovery == NULL) {
        return BRINGUP_RECOVERY_ACTION_NONE;
    }
    if (recovery->phase == BRINGUP_RECOVERY_PHASE_STOPPING) {
        (void)done;
        return recovery_start_next(recovery);
    }
    if ((recovery->phase == BRINGUP_RECOVERY_PHASE_STARTING ||
         recovery->phase == BRINGUP_RECOVERY_PHASE_WAITING_FIRST_FRAME) &&
        done == BRINGUP_WORKER_DONE_INIT_FAILED) {
        return BRINGUP_RECOVERY_ACTION_FAILURE | recovery_start_next(recovery);
    }
    return BRINGUP_RECOVERY_ACTION_NONE;
}
