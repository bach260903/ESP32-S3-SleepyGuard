#include <assert.h>
#include <stdio.h>

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
    assert(bringup_recovery_on_frame(&r, true, 100) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_frame(&r, false, 101) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(r.consecutive_invalid == 1);
    assert(bringup_recovery_on_frame(&r, true, 102) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(r.consecutive_invalid == 0);
    assert(bringup_recovery_on_frame(&r, false, 103) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_frame(&r, false, 104) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_frame(&r, false, 105) & BRINGUP_RECOVERY_ACTION_REQUEST_STOP);
    assert(r.phase == BRINGUP_RECOVERY_PHASE_STOPPING);
    assert(r.remaining == 3);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_STOPPED, 200) & BRINGUP_RECOVERY_ACTION_START_WORKER);
    assert(r.remaining == 2);
    assert(bringup_recovery_on_worker_ready(&r, 300) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(r.phase == BRINGUP_RECOVERY_PHASE_WAITING_FIRST_FRAME);
    assert(bringup_recovery_on_tick(&r, 5298) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_frame(&r, true, 5299) & BRINGUP_RECOVERY_ACTION_SUCCESS);
    assert(r.phase == BRINGUP_RECOVERY_PHASE_RUNNING);
    assert(bringup_recovery_on_frame(&r, false, 5301) == BRINGUP_RECOVERY_ACTION_NONE);
    for (uint64_t t = 10000; t < 65299; t += 4000) {
        assert(bringup_recovery_on_frame(&r, true, t) == BRINGUP_RECOVERY_ACTION_NONE);
    }
    assert(bringup_recovery_on_frame(&r, true, 65298) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_tick(&r, 65298) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(r.remaining == 2);
    assert(bringup_recovery_on_tick(&r, 65299) & BRINGUP_RECOVERY_ACTION_BUDGET_RESET);
    assert(r.remaining == 3);

    c.first_frame_timeout_ms = 3000;
    bringup_recovery_init(&r, &c, 0);
    assert(bringup_recovery_on_tick(&r, 4999) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_tick(&r, 5000) & BRINGUP_RECOVERY_ACTION_REQUEST_STOP);
    assert(r.phase == BRINGUP_RECOVERY_PHASE_STOPPING);

    bringup_recovery_init(&r, &c, 0);
    assert(bringup_recovery_on_tick(&r, 5000) & BRINGUP_RECOVERY_ACTION_REQUEST_STOP);
    assert(bringup_recovery_on_worker_done(&r, BRINGUP_WORKER_DONE_STOPPED, 5001) &
           BRINGUP_RECOVERY_ACTION_START_WORKER);
    assert(bringup_recovery_on_worker_ready(&r, 9000) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_tick(&r, 11999) == BRINGUP_RECOVERY_ACTION_NONE);
    assert(bringup_recovery_on_frame(&r, true, 12000) &
           BRINGUP_RECOVERY_ACTION_REQUEST_STOP);
    assert(r.phase == BRINGUP_RECOVERY_PHASE_STOPPING);

    puts("RECOVERY_LOGIC PASS");
    return 0;
}
