#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

static const bringup_recovery_config_t recovery_config = {
    .recovery_attempts = BRINGUP_RECOVERY_ATTEMPTS,
    .invalid_frame_limit = BRINGUP_CONSECUTIVE_INVALID_LIMIT,
    .frame_loss_timeout_ms = BRINGUP_FRAME_LOSS_TIMEOUT_MS,
    .first_frame_timeout_ms = BRINGUP_RECOVERY_FIRST_FRAME_TIMEOUT_MS,
    .stable_reset_ms = BRINGUP_STABLE_RESET_MS,
    .stop_wait_limit_ms = BRINGUP_STOP_WAIT_LIMIT_MS,
    .recovery_wait_period_ms = BRINGUP_RECOVERY_WAIT_PERIOD_MS,
};

static void report_interrupted_window(bringup_measurement_t *window,
                                      uint64_t at_ms)
{
    if (window->valid == 0U && window->invalid == 0U) {
        return;
    }
    bringup_measurement_t report;
    if (bringup_measurement_finish(window, at_ms, &report)) {
        bringup_log_frame_report(report.width, report.height, report.valid,
                                 report.invalid, report.elapsed_ms, report.fps);
    }
}

static void apply_recovery_action(bringup_recovery_t *recovery,
                                  bringup_recovery_action_t action,
                                  uint64_t at_ms, const char *reason)
{
    if (action & (BRINGUP_RECOVERY_ACTION_FAILURE |
                  BRINGUP_RECOVERY_ACTION_REQUEST_STOP)) {
        recovery->last_cause = reason;
    }
    if ((action & BRINGUP_RECOVERY_ACTION_FAULT) &&
        recovery->fault_code != NULL &&
        strcmp(recovery->fault_code, "WORKER_STOP_TIMEOUT") == 0) {
        recovery->last_cause = "worker_stop_timeout";
    }
    if (action & BRINGUP_RECOVERY_ACTION_FAILURE) {
        const unsigned result_remaining = recovery->remaining +
            ((action & BRINGUP_RECOVERY_ACTION_START_WORKER) ? 1U : 0U);
        bringup_log_recovery("failure", result_remaining, reason);
    }
    if (action & BRINGUP_RECOVERY_ACTION_REQUEST_STOP) {
        bringup_log_recovery("start", recovery->remaining, reason);
        ESP_LOGI(TAG, "HEAP stage=recovery_start psram_heap_free_bytes=%u internal_heap_free_bytes=%u",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        bringup_camera_request_stop();
    }
    if (action & BRINGUP_RECOVERY_ACTION_REPORT_WAIT) {
        ESP_LOGW(TAG, "RECOVERY_WAIT phase=stopping waited_ms=%llu budget_remaining=%u",
                 (unsigned long long)(at_ms - recovery->stop_requested_ms),
                 recovery->remaining);
    }
    if (action & BRINGUP_RECOVERY_ACTION_START_WORKER) {
        bringup_log_recovery("attempt_start", recovery->remaining, reason);
        const unsigned attempt = recovery->config.recovery_attempts -
                                 recovery->remaining;
        QueueHandle_t started = bringup_camera_start(attempt, true);
        if (started == NULL) {
            bringup_log_camera_init("recovery", 0U, attempt, false,
                                    NULL, ESP_ERR_NO_MEM);
            const bringup_recovery_action_t failure =
                bringup_recovery_on_worker_done(recovery,
                                               BRINGUP_WORKER_DONE_INIT_FAILED,
                                               at_ms);
            apply_recovery_action(recovery, failure, at_ms, "worker_start_failed");
        }
    }
    if (action & BRINGUP_RECOVERY_ACTION_SUCCESS) {
        bringup_log_recovery("success", recovery->remaining, "valid_frame");
    }
    if (action & BRINGUP_RECOVERY_ACTION_BUDGET_RESET) {
        bringup_log_recovery("budget_reset", recovery->remaining, "stable_60s");
    }
    if (action & BRINGUP_RECOVERY_ACTION_FAULT &&
        recovery->fault_code != NULL &&
        recovery->fault_code[0] == 'R') {
        bringup_log_recovery("exhausted", recovery->remaining, "budget_empty");
    }
}

static QueueHandle_t start_camera_attempt(bringup_startup_t *startup)
{
    while (startup->state == BRINGUP_STATE_INIT) {
        const unsigned attempt = bringup_startup_begin_attempt(startup);
        if (attempt == 0U) {
            break;
        }
        QueueHandle_t events = bringup_camera_start(attempt, false);
        if (events != NULL) {
            return events;
        }
        bringup_log_camera_init("boot", 0U, attempt, false,
                                NULL, ESP_ERR_NO_MEM);
        bringup_startup_camera_result(startup, false, now_ms());
    }
    return NULL;
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

    bringup_startup_t startup;
    bringup_startup_init(&startup, BRINGUP_INITIAL_INIT_ATTEMPTS);

    const char *psram_reason = NULL;
    const char *psram_error = bringup_psram_check(&psram_reason);
    if (psram_error != NULL) {
        bringup_startup_psram_failure(&startup, psram_error, psram_reason, now_ms());
    }

    QueueHandle_t camera_events = startup.state == BRINGUP_STATE_INIT
                                      ? start_camera_attempt(&startup) : NULL;

    bringup_measurement_t window;
    bringup_recovery_t recovery = {0};
    bool recovery_active = false;
    bool recovery_fault_reported = false;
    uint64_t recovery_fault_last_report_ms = 0;
    for (;;) {
        bringup_camera_event_t event;
        if (camera_events != NULL &&
            xQueueReceive(camera_events, &event,
                          pdMS_TO_TICKS(BRINGUP_SUPERVISOR_POLL_MS)) == pdTRUE) {
            if (event.kind == BRINGUP_CAMERA_READY) {
                if (startup.state == BRINGUP_STATE_INIT) {
                    bringup_startup_camera_result(&startup, true, event.at_ms);
                    bringup_measurement_start(&window, event.at_ms,
                                              BRINGUP_FRAME_WIDTH, BRINGUP_FRAME_HEIGHT);
                    bringup_recovery_init(&recovery, &recovery_config, event.at_ms);
                    recovery_active = true;
                } else if (recovery_active) {
                    (void)bringup_recovery_on_worker_ready(&recovery, event.at_ms);
                }
            } else if (event.kind == BRINGUP_CAMERA_INIT_FAILED) {
                if (startup.state == BRINGUP_STATE_INIT) {
                    bringup_startup_camera_result(&startup, false, event.at_ms);
                    if (startup.state == BRINGUP_STATE_INIT) {
                        camera_events = start_camera_attempt(&startup);
                    }
                } else if (recovery_active) {
                    const bringup_recovery_action_t action =
                        bringup_recovery_on_worker_done(&recovery,
                                                       BRINGUP_WORKER_DONE_INIT_FAILED,
                                                       event.at_ms);
                    apply_recovery_action(&recovery, action, event.at_ms,
                                          "camera_init_failed");
                }
            } else if (event.kind == BRINGUP_CAMERA_STOPPED && recovery_active) {
                const bringup_recovery_action_t action =
                    bringup_recovery_on_worker_done(&recovery,
                                                   BRINGUP_WORKER_DONE_STOPPED,
                                                   event.at_ms);
                apply_recovery_action(&recovery, action, event.at_ms,
                                      "worker_stopped");
            } else if (startup.state == BRINGUP_STATE_RUNNING &&
                       event.kind == BRINGUP_CAMERA_FRAME) {
                const bringup_frame_observation_t frame = {
                    .at_ms = event.at_ms,
                    .valid = event.valid,
                    .width = event.width,
                    .height = event.height,
                };
                const bringup_recovery_phase_t prior_phase = recovery.phase;
                if (prior_phase == BRINGUP_RECOVERY_PHASE_RUNNING) {
                    bringup_measurement_record(&window, &frame);
                }
                if (prior_phase == BRINGUP_RECOVERY_PHASE_RUNNING ||
                    prior_phase == BRINGUP_RECOVERY_PHASE_WAITING_FIRST_FRAME) {
                    if (!event.valid) {
                        bringup_log_frame_unavailable(
                            prior_phase == BRINGUP_RECOVERY_PHASE_RUNNING ? "running" : "recovering",
                            event.reason == BRINGUP_FRAME_REASON_FB_GET_NULL ? "fb_get_null" : "invalid_size",
                            event.raw_error);
                    }
                    const bringup_recovery_action_t action =
                        bringup_recovery_on_frame(&recovery, event.valid, event.at_ms);
                    if (action & BRINGUP_RECOVERY_ACTION_REQUEST_STOP &&
                        prior_phase == BRINGUP_RECOVERY_PHASE_RUNNING) {
                        report_interrupted_window(&window, event.at_ms);
                    }
                    if (action & BRINGUP_RECOVERY_ACTION_SUCCESS) {
                        bringup_measurement_start(&window, event.at_ms,
                                                  BRINGUP_FRAME_WIDTH, BRINGUP_FRAME_HEIGHT);
                        bringup_measurement_record(&window, &frame);
                    }
                    apply_recovery_action(&recovery, action, event.at_ms,
                                          prior_phase == BRINGUP_RECOVERY_PHASE_WAITING_FIRST_FRAME
                                              ? "first_frame_timeout"
                                              : (recovery.consecutive_invalid >=
                                                 recovery.config.invalid_frame_limit
                                                     ? "invalid_frames" : "frame_timeout"));
                }
            }
        } else if (camera_events == NULL) {
            vTaskDelay(pdMS_TO_TICKS(BRINGUP_SUPERVISOR_POLL_MS));
        }
        if (bringup_startup_fault_due(&startup, now_ms(),
                                      BRINGUP_FAULT_REPORT_PERIOD_MS)) {
            bringup_log_fault(startup.fault_code, startup.fault_reason,
                              startup.fault_reason, 0U);
        }
        if (recovery_active) {
            const uint64_t tick_ms = now_ms();
            const bringup_recovery_action_t action =
                bringup_recovery_on_tick(&recovery, tick_ms);
            if (action & BRINGUP_RECOVERY_ACTION_REQUEST_STOP) {
                report_interrupted_window(&window, tick_ms);
            }
            apply_recovery_action(&recovery, action, tick_ms,
                                  recovery.phase == BRINGUP_RECOVERY_PHASE_STOPPING &&
                                          recovery.worker_ready_ms > recovery.running_since_ms
                                      ? "first_frame_timeout" : "frame_timeout");
            if (recovery.phase == BRINGUP_RECOVERY_PHASE_FAULT &&
                (!recovery_fault_reported ||
                 (tick_ms >= recovery_fault_last_report_ms &&
                  tick_ms - recovery_fault_last_report_ms >=
                      BRINGUP_FAULT_REPORT_PERIOD_MS))) {
                bringup_log_fault(recovery.fault_code, recovery.fault_code,
                                  recovery.last_cause != NULL
                                      ? recovery.last_cause : "unknown",
                                  recovery.remaining);
                recovery_fault_last_report_ms = tick_ms;
                recovery_fault_reported = true;
            }
        }
        if (startup.state == BRINGUP_STATE_RUNNING && recovery_active &&
            recovery.phase == BRINGUP_RECOVERY_PHASE_RUNNING &&
            bringup_measurement_due(&window, now_ms(),
                                    BRINGUP_FRAME_REPORT_PERIOD_MS)) {
            bringup_measurement_t report;
            if (bringup_measurement_finish(&window, now_ms(), &report)) {
                bringup_log_frame_report(report.width, report.height, report.valid,
                                         report.invalid, report.elapsed_ms, report.fps);
            }
        }
    }
}
