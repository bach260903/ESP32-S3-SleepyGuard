#include "bringup_psram.h"

#include <stddef.h>
#include <stdint.h>

#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "bringup_config.h"
#include "bringup_log.h"

static uint8_t address_pattern(size_t index)
{
    return (uint8_t)((index * 37U) ^ (index >> 8U) ^ 0xA5U);
}

const char *bringup_psram_check(const char **reason)
{
    if (reason != NULL) {
        *reason = "unknown";
    }
#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_PSRAM_UNAVAILABLE
    ESP_LOGW("bringup", "FAULT_SCENARIO name=PSRAM_UNAVAILABLE");
#endif
    if (!esp_psram_is_initialized()) {
        if (reason != NULL) { *reason = "not_initialized"; }
        bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", false,
                               "PSRAM_UNAVAILABLE");
        return "PSRAM_UNAVAILABLE";
    }
#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_PSRAM_UNAVAILABLE
    if (reason != NULL) { *reason = "injected_unavailable"; }
    bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", false,
                           "PSRAM_UNAVAILABLE");
    return "PSRAM_UNAVAILABLE";
#endif
#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_PSRAM_MISMATCH
    ESP_LOGW("bringup", "FAULT_SCENARIO name=PSRAM_MISMATCH");
    if (reason != NULL) { *reason = "injected_capacity_mismatch"; }
    bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", false,
                           "PSRAM_CONFIG_MISMATCH");
    return "PSRAM_CONFIG_MISMATCH";
#endif
    if (esp_psram_get_size() != BRINGUP_PSRAM_EXPECTED_BYTES) {
        if (reason != NULL) { *reason = "capacity_mismatch"; }
        bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", false,
                               "PSRAM_CONFIG_MISMATCH");
        return "PSRAM_CONFIG_MISMATCH";
    }

#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_PSRAM_ALLOCATE
    ESP_LOGW("bringup", "FAULT_SCENARIO name=PSRAM_ALLOCATE");
    if (reason != NULL) { *reason = "injected_allocation_failed"; }
    bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", false,
                           "PSRAM_TEST_FAILED");
    return "PSRAM_TEST_FAILED";
#endif
    uint8_t *buffer = heap_caps_malloc(BRINGUP_PSRAM_TEST_BYTES,
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buffer == NULL) {
        if (reason != NULL) { *reason = "allocation_failed"; }
        bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", false,
                               "PSRAM_TEST_FAILED");
        return "PSRAM_TEST_FAILED";
    }

    bool passed = true;
    for (unsigned pass = 0; pass < 2U && passed; ++pass) {
        for (size_t i = 0; i < BRINGUP_PSRAM_TEST_BYTES; ++i) {
            const uint8_t value = address_pattern(i);
            buffer[i] = pass == 0U ? value : (uint8_t)~value;
        }
#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_PSRAM_READBACK
        if (pass == 0U) {
            ESP_LOGW("bringup", "FAULT_SCENARIO name=PSRAM_READBACK");
            buffer[BRINGUP_PSRAM_TEST_BYTES / 2U] ^= 1U;
        }
#endif
        for (size_t i = 0; i < BRINGUP_PSRAM_TEST_BYTES; ++i) {
            const uint8_t value = address_pattern(i);
            if (buffer[i] != (pass == 0U ? value : (uint8_t)~value)) {
                passed = false;
                break;
            }
        }
    }

    heap_caps_free(buffer);
    if (!passed && reason != NULL) {
#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_PSRAM_READBACK
        *reason = "injected_readback_mismatch";
#else
        *reason = "readback_mismatch";
#endif
    }
    bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", passed,
                           passed ? "none" : "PSRAM_TEST_FAILED");
    return passed ? NULL : "PSRAM_TEST_FAILED";
}
