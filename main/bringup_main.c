#include <stdbool.h>
#include <stdint.h>

#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "bringup_camera.h"
#include "bringup_config.h"
#include "bringup_log.h"
#include "bringup_logic.h"
#include "bringup_psram.h"

static const char *TAG = "bringup";

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

void app_main(void)
{
    const bool psram_initialized = esp_psram_is_initialized();
    const size_t psram_size_raw_bytes = esp_psram_get_size();
    const size_t psram_heap_free_bytes = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    uint32_t flash_physical_bytes = 0;
    const esp_err_t flash_result = esp_flash_get_physical_size(
        esp_flash_default_chip, &flash_physical_bytes);

    bringup_log_boot_info(psram_initialized, psram_size_raw_bytes,
                          BRINGUP_PSRAM_EXPECTED_BYTES, psram_heap_free_bytes,
                          flash_physical_bytes, BRINGUP_FLASH_EXPECTED_BYTES);
    if (flash_result != ESP_OK) {
        ESP_LOGE(TAG, "BOOT_INFO flash_error=%s", esp_err_to_name(flash_result));
    }

    const char *psram_error = bringup_psram_check();
    if (psram_error != NULL) {
        bringup_log_fault(psram_error, "psram_check", 0U);
        return;
    }

    QueueHandle_t camera_events = bringup_camera_start();
    if (camera_events == NULL) {
        ESP_LOGE(TAG, "CAM_INIT_FAILED stage=worker_start raw_error=0x0");
        return;
    }

    bool running = false;
    bringup_measurement_t window;
    for (;;) {
        bringup_camera_event_t event;
        if (xQueueReceive(camera_events, &event,
                          pdMS_TO_TICKS(BRINGUP_SUPERVISOR_POLL_MS)) == pdTRUE) {
            if (event.kind == BRINGUP_CAMERA_READY) {
                bringup_measurement_start(&window, event.at_ms,
                                          BRINGUP_FRAME_WIDTH, BRINGUP_FRAME_HEIGHT);
                running = true;
            } else if (event.kind == BRINGUP_CAMERA_INIT_FAILED) {
                ESP_LOGE(TAG, "FAULT code=CAM_INIT_FAILED reason=camera_init raw_error=0x%x",
                         (unsigned)event.raw_error);
                return;
            } else if (running && event.kind == BRINGUP_CAMERA_FRAME) {
                const bringup_frame_observation_t frame = {
                    .at_ms = event.at_ms,
                    .valid = event.valid,
                    .width = event.width,
                    .height = event.height,
                };
                bringup_measurement_record(&window, &frame);
                if (!event.valid) {
                    bringup_log_frame_unavailable("running",
                                                  event.bytes == 0U ? "empty_frame" : "invalid_frame",
                                                  0);
                }
            }
        }
        if (running && bringup_measurement_due(&window, now_ms(),
                                                BRINGUP_FRAME_REPORT_PERIOD_MS)) {
            bringup_measurement_t report;
            if (bringup_measurement_finish(&window, now_ms(), &report)) {
                bringup_log_frame_report(report.width, report.height, report.valid,
                                         report.invalid, report.elapsed_ms, report.fps);
            }
        }
    }
}
