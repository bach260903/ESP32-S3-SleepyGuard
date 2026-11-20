#include "bringup_log.h"

#include <inttypes.h>

#include "esp_log.h"

static const char *TAG = "bringup";

void bringup_log_boot_info(bool psram_initialized, size_t psram_raw_bytes,
                           size_t psram_expected_bytes, size_t psram_heap_free_bytes,
                           uint32_t flash_physical_bytes, uint32_t flash_expected_bytes)
{
    ESP_LOGI(TAG, "BOOT_INFO psram_initialized=%d psram_size_raw_bytes=%zu "
             "psram_expected_bytes=%zu psram_heap_free_bytes=%zu "
             "flash_physical=%" PRIu32 " flash_expected=%" PRIu32,
             psram_initialized, psram_raw_bytes, psram_expected_bytes,
             psram_heap_free_bytes, flash_physical_bytes, flash_expected_bytes);
}

void bringup_log_psram_test(size_t test_bytes, const char *patterns, bool passed,
                            const char *error_code)
{
    if (passed) {
        ESP_LOGI(TAG, "PSRAM_TEST test_bytes=%zu patterns=%s result=PASS",
                 test_bytes, patterns);
    } else {
        ESP_LOGE(TAG, "PSRAM_TEST test_bytes=%zu patterns=%s result=FAIL code=%s",
                 test_bytes, patterns, error_code);
    }
}

void bringup_log_camera_init(unsigned attempt, bool success, const char *sensor,
                             int raw_error)
{
    if (success) {
        ESP_LOGI(TAG, "CAMERA_INIT attempt=%u result=success sensor=%s", attempt, sensor);
    } else {
        ESP_LOGE(TAG, "CAMERA_INIT attempt=%u result=failure raw_error=0x%x code=CAM_INIT_FAILED",
                 attempt, (unsigned)raw_error);
    }
}

void bringup_log_frame_report(unsigned width, unsigned height, uint32_t valid,
                              uint32_t invalid, uint64_t elapsed_ms, double fps)
{
    ESP_LOGI(TAG, "FRAME_REPORT width=%u height=%u valid=%" PRIu32
             " invalid=%" PRIu32 " elapsed_ms=%" PRIu64 " fps=%.2f",
             width, height, valid, invalid, elapsed_ms, fps);
}

void bringup_log_frame_unavailable(const char *stage, const char *reason,
                                   int raw_error)
{
    ESP_LOGE(TAG, "FRAME_UNAVAILABLE stage=%s reason=%s raw_error=0x%x",
             stage, reason, (unsigned)raw_error);
}

void bringup_log_recovery(const char *stage, unsigned remaining, const char *reason)
{
    ESP_LOGW(TAG, "RECOVERY stage=%s remaining=%u reason=%s", stage, remaining, reason);
}

void bringup_log_fault(const char *code, const char *reason, unsigned remaining)
{
    ESP_LOGE(TAG, "FAULT code=%s reason=%s remaining=%u", code, reason, remaining);
}
