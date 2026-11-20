#include <stdio.h>
#include <string.h>

#include "bringup_config.h"
#include "bringup_logic.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line=%d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int camera_exhaustion(void)
{
    bringup_startup_t startup;
    bringup_startup_init(&startup, BRINGUP_INITIAL_INIT_ATTEMPTS);
    CHECK(startup.state == BRINGUP_STATE_INIT);
    for (unsigned attempt = 1; attempt <= BRINGUP_INITIAL_INIT_ATTEMPTS; ++attempt) {
        CHECK(bringup_startup_begin_attempt(&startup) == attempt);
        bringup_startup_camera_result(&startup, false, 100U * attempt);
    }
    CHECK(startup.state == BRINGUP_STATE_FAULT);
    CHECK(startup.attempts_used == BRINGUP_INITIAL_INIT_ATTEMPTS);
    CHECK(strcmp(startup.fault_code, "RECOVERY_EXHAUSTED") == 0);
    CHECK(strcmp(startup.fault_reason, "camera_init_exhausted") == 0);
    CHECK(bringup_startup_begin_attempt(&startup) == 0U);
    bringup_startup_camera_result(&startup, true, 301U);
    CHECK(startup.state == BRINGUP_STATE_FAULT);

    CHECK(bringup_startup_fault_due(&startup, 300U, BRINGUP_FAULT_REPORT_PERIOD_MS));
    CHECK(!bringup_startup_fault_due(&startup, 5299U, BRINGUP_FAULT_REPORT_PERIOD_MS));
    CHECK(bringup_startup_fault_due(&startup, 5300U, BRINGUP_FAULT_REPORT_PERIOD_MS));
    CHECK(!bringup_startup_fault_due(&startup, 10299U, BRINGUP_FAULT_REPORT_PERIOD_MS));
    CHECK(bringup_startup_fault_due(&startup, 10300U, BRINGUP_FAULT_REPORT_PERIOD_MS));
    return 0;
}

static int psram_failure(void)
{
    bringup_startup_t startup;
    bringup_startup_init(&startup, BRINGUP_INITIAL_INIT_ATTEMPTS);
    bringup_startup_psram_failure(&startup, "PSRAM_UNAVAILABLE", "not_initialized", 42U);
    CHECK(startup.state == BRINGUP_STATE_FAULT);
    CHECK(startup.attempts_used == 0U);
    CHECK(bringup_startup_begin_attempt(&startup) == 0U);
    CHECK(strcmp(startup.fault_code, "PSRAM_UNAVAILABLE") == 0);
    CHECK(bringup_startup_fault_due(&startup, 42U, BRINGUP_FAULT_REPORT_PERIOD_MS));
    return 0;
}

static int camera_recovers_early(void)
{
    bringup_startup_t startup;
    bringup_startup_init(&startup, BRINGUP_INITIAL_INIT_ATTEMPTS);
    CHECK(bringup_startup_begin_attempt(&startup) == 1U);
    bringup_startup_camera_result(&startup, false, 100U);
    CHECK(bringup_startup_begin_attempt(&startup) == 2U);
    bringup_startup_camera_result(&startup, true, 200U);
    CHECK(startup.state == BRINGUP_STATE_RUNNING);
    CHECK(bringup_startup_begin_attempt(&startup) == 0U);
    CHECK(!bringup_startup_fault_due(&startup, 6000U, BRINGUP_FAULT_REPORT_PERIOD_MS));
    return 0;
}

int main(void)
{
    if (camera_exhaustion() || psram_failure() || camera_recovers_early()) {
        return 1;
    }
    puts("STARTUP_FAULTS PASS");
    return 0;
}
