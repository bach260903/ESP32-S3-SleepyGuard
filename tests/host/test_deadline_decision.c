#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bringup_config.h"
#include "bringup_logic.h"

static bringup_recovery_config_t config(void)
{
    return (bringup_recovery_config_t){
        .recovery_attempts = BRINGUP_RECOVERY_ATTEMPTS,
        .invalid_frame_limit = BRINGUP_CONSECUTIVE_INVALID_LIMIT,
        .frame_loss_timeout_ms = BRINGUP_FRAME_LOSS_TIMEOUT_MS,
        .first_frame_timeout_ms = BRINGUP_RECOVERY_FIRST_FRAME_TIMEOUT_MS,
        .stable_reset_ms = BRINGUP_STABLE_RESET_MS,
        .stop_wait_limit_ms = BRINGUP_STOP_WAIT_LIMIT_MS,
        .recovery_wait_period_ms = BRINGUP_RECOVERY_WAIT_PERIOD_MS,
    };
}

int main(void)
{
    bringup_recovery_t r;
    bringup_recovery_config_t c = config();
    bringup_recovery_init(&r, &c, 0);
    assert(bringup_recovery_on_tick(&r, 4999) == BRINGUP_RECOVERY_ACTION_NONE);
    const bringup_recovery_action_t deadline = bringup_recovery_on_tick(&r, 5000);
    assert(deadline & BRINGUP_RECOVERY_ACTION_REQUEST_STOP);
    assert(deadline & BRINGUP_RECOVERY_ACTION_REPORT_WAIT);
    assert(bringup_recovery_on_frame(&r, true, 5001) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_worker_ready(&r, 5002) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(r.phase == BRINGUP_RECOVERY_PHASE_STOPPING);
    assert(bringup_recovery_on_tick(&r, 9999) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_tick(&r, 10000) & BRINGUP_RECOVERY_ACTION_REPORT_WAIT);
    assert(bringup_recovery_on_tick(&r, 19999) != BRINGUP_RECOVERY_ACTION_FAULT);
    assert(bringup_recovery_on_tick(&r, 20000) & BRINGUP_RECOVERY_ACTION_FAULT);
    assert(strcmp(r.fault_code, "WORKER_STOP_TIMEOUT") == 0);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_STOPPED, 20001) == BRINGUP_RECOVERY_ACTION_NONE);

    bringup_recovery_init(&r, &c, 0);
    assert(bringup_recovery_on_tick(&r, 5000) & BRINGUP_RECOVERY_ACTION_REQUEST_STOP);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_INIT_FAILED, 5001) & BRINGUP_RECOVERY_ACTION_START_WORKER);
    assert(r.remaining == 2);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_INIT_FAILED, 5002) & BRINGUP_RECOVERY_ACTION_START_WORKER);
    assert(r.remaining == 1);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_INIT_FAILED, 5003) & BRINGUP_RECOVERY_ACTION_START_WORKER);
    assert(r.remaining == 0);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_INIT_FAILED, 5004) & BRINGUP_RECOVERY_ACTION_FAULT);
    assert(strcmp(r.fault_code, "RECOVERY_EXHAUSTED") == 0);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_INIT_FAILED, 5005) ==
           BRINGUP_RECOVERY_ACTION_NONE);

    bringup_recovery_init(&r, &c, 0);
    r.remaining = 0;
    assert(bringup_recovery_on_tick(&r, 5000) & BRINGUP_RECOVERY_ACTION_REQUEST_STOP);
    assert(bringup_recovery_on_frame(&r, true, 5001) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_STOPPED, 5002) &
           BRINGUP_RECOVERY_ACTION_FAULT);
    assert(strcmp(r.fault_code, "RECOVERY_EXHAUSTED") == 0);

    puts("DEADLINE_DECISION PASS");
    return 0;
}
