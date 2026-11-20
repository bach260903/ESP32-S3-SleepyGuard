#include "bringup_psram.h"

#include <stddef.h>
#include <stdint.h>

#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "bringup_config.h"
#include "bringup_log.h"

static uint8_t address_pattern(size_t index)
{
    return (uint8_t)((index * 37U) ^ (index >> 8U) ^ 0xA5U);
}

const char *bringup_psram_check(void)
{
    if (!esp_psram_is_initialized()) {
        bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", false,
                               "PSRAM_UNAVAILABLE");
        return "PSRAM_UNAVAILABLE";
    }
    if (esp_psram_get_size() != BRINGUP_PSRAM_EXPECTED_BYTES) {
        bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", false,
                               "PSRAM_CONFIG_MISMATCH");
        return "PSRAM_CONFIG_MISMATCH";
    }

    uint8_t *buffer = heap_caps_malloc(BRINGUP_PSRAM_TEST_BYTES,
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buffer == NULL) {
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
        for (size_t i = 0; i < BRINGUP_PSRAM_TEST_BYTES; ++i) {
            const uint8_t value = address_pattern(i);
            if (buffer[i] != (pass == 0U ? value : (uint8_t)~value)) {
                passed = false;
                break;
            }
        }
    }

    heap_caps_free(buffer);
    bringup_log_psram_test(BRINGUP_PSRAM_TEST_BYTES, "address,inverse", passed,
                           passed ? "none" : "PSRAM_TEST_FAILED");
    return passed ? NULL : "PSRAM_TEST_FAILED";
}
