#include "bringup_camera.h"

#include <stdbool.h>
#include <stdatomic.h>

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "board_pins.h"
#include "bringup_config.h"
#include "bringup_log.h"

static QueueHandle_t camera_events;
static atomic_bool stop_requested = ATOMIC_VAR_INIT(false);
static atomic_bool worker_active = ATOMIC_VAR_INIT(false);
static unsigned worker_sequence;
static unsigned active_worker_index;

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static void log_heap(const char *stage, unsigned worker_index)
{
    ESP_LOGI("bringup", "HEAP stage=%s worker=%u psram_heap_free_bytes=%u internal_heap_free_bytes=%u",
             stage, worker_index,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
}

static void send_event(const bringup_camera_event_t *event)
{
    xQueueSend(camera_events, event, portMAX_DELAY);
}

static void worker_finish(bringup_camera_event_kind_t kind, int raw_error,
                          bool driver_initialized)
{
    if (driver_initialized) {
        const esp_err_t result = esp_camera_deinit();
        ESP_LOGI("bringup", "WORKER_EVENT deinit_done at_ms=%llu result=%s",
                 (unsigned long long)now_ms(), esp_err_to_name(result));
    }
    atomic_store(&worker_active, false);
    const bringup_camera_event_t terminal = {
        .kind = kind, .at_ms = now_ms(), .raw_error = raw_error,
    };
    ESP_LOGI("bringup", "WORKER_EVENT terminal=%s at_ms=%llu",
             kind == BRINGUP_CAMERA_STOPPED ? "STOPPED" : "INIT_FAILED",
             (unsigned long long)terminal.at_ms);
    send_event(&terminal);
    vTaskDelete(NULL);
}

static void camera_worker(void *argument)
{
    const unsigned attempt = (unsigned)(uintptr_t)argument;
    const unsigned worker_index = active_worker_index;
#if CONFIG_BRINGUP_FAULT_INJECTION
    const bool fault_selected = CONFIG_BRINGUP_FAULT_WORKER_INDEX == 0 ||
                                (unsigned)CONFIG_BRINGUP_FAULT_WORKER_INDEX == worker_index;
    bool first_valid_seen = false;
    uint64_t first_valid_ms = 0;
    bool delayed_once = false;
    unsigned invalid_remaining =
#if CONFIG_BRINGUP_FAULT_SCENARIO_THREE_INVALID
        3U;
#elif CONFIG_BRINGUP_FAULT_SCENARIO_SINGLE_INVALID
        1U;
#else
        0U;
#endif
    ESP_LOGI("bringup", "FAULT_SCENARIO worker=%u selected=%u after_first_valid_s=%d",
             worker_index, fault_selected, CONFIG_BRINGUP_FAULT_AFTER_FIRST_VALID_S);
#else
    (void)worker_index;
#endif
    const camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SDA,
        .pin_sccb_scl = CAM_PIN_SCL,
        .pin_d0 = CAM_PIN_D0,
        .pin_d1 = CAM_PIN_D1,
        .pin_d2 = CAM_PIN_D2,
        .pin_d3 = CAM_PIN_D3,
        .pin_d4 = CAM_PIN_D4,
        .pin_d5 = CAM_PIN_D5,
        .pin_d6 = CAM_PIN_D6,
        .pin_d7 = CAM_PIN_D7,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = BRINGUP_CAMERA_XCLK_HZ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = BRINGUP_CAMERA_PIXEL_FORMAT,
        .frame_size = BRINGUP_CAMERA_FRAME_SIZE,
        .jpeg_quality = BRINGUP_CAMERA_JPEG_QUALITY,
        .fb_count = BRINGUP_CAMERA_FB_COUNT,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = BRINGUP_CAMERA_GRAB_MODE,
    };

#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_CAMERA_INIT
    ESP_LOGW("bringup", "FAULT_SCENARIO name=CAMERA_INIT attempt=%u", attempt);
    bringup_log_camera_init(attempt, false, NULL, ESP_FAIL);
    worker_finish(BRINGUP_CAMERA_INIT_FAILED, ESP_FAIL, false);
    return;
#endif
    const esp_err_t result = esp_camera_init(&config);
    sensor_t *sensor = result == ESP_OK ? esp_camera_sensor_get() : NULL;
    if (result != ESP_OK || sensor == NULL || sensor->id.PID != OV3660_PID) {
        const int raw_error = result != ESP_OK ? result : (sensor == NULL ? -1 : sensor->id.PID);
        bringup_log_camera_init(attempt, false, NULL, raw_error);
        worker_finish(BRINGUP_CAMERA_INIT_FAILED, raw_error, result == ESP_OK);
        return;
    }

#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_RECOVERY_INIT
    if (fault_selected && worker_index > 1U) {
        ESP_LOGW("bringup", "FAULT_SCENARIO name=RECOVERY_INIT worker=%u", worker_index);
        log_heap("driver_init_before_injected_failure", worker_index);
        bringup_log_camera_init(attempt, false, NULL, ESP_FAIL);
        worker_finish(BRINGUP_CAMERA_INIT_FAILED, ESP_FAIL, true);
        return;
    }
#endif
    bringup_log_camera_init(attempt, true, "OV3660", 0);
    log_heap("init_success", worker_index);
    const bringup_camera_event_t ready = { .kind = BRINGUP_CAMERA_READY, .at_ms = now_ms() };
    send_event(&ready);

#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_INIT_NO_FRAME
    if (fault_selected && worker_index > 1U) {
        ESP_LOGW("bringup", "FAULT_SCENARIO name=INIT_NO_FRAME worker=%u", worker_index);
        vTaskDelay(pdMS_TO_TICKS(BRINGUP_FAULT_DELAY_MS));
    }
#endif

    for (;;) {
        if (atomic_load(&stop_requested)) {
            break;
        }
        const uint64_t fb_get_started_ms = now_ms();
        camera_fb_t *frame = esp_camera_fb_get();
        const uint64_t fb_get_elapsed_ms = now_ms() - fb_get_started_ms;
#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_HOLD_FRAME
        if (fault_selected) {
            ESP_LOGI("bringup", "WORKER_EVENT fb_get_elapsed_ms=%llu worker=%u",
                     (unsigned long long)fb_get_elapsed_ms, worker_index);
        }
#else
        (void)fb_get_elapsed_ms;
#endif
        bringup_camera_event_t event = {
            .kind = BRINGUP_CAMERA_FRAME, .at_ms = now_ms(),
            .reason = frame == NULL ? BRINGUP_FRAME_REASON_FB_GET_NULL
                                    : BRINGUP_FRAME_REASON_NONE,
        };
        if (frame != NULL) {
            event.width = frame->width;
            event.height = frame->height;
            event.bytes = frame->len;
            event.valid = frame->buf != NULL && frame->width == BRINGUP_FRAME_WIDTH &&
                          frame->height == BRINGUP_FRAME_HEIGHT &&
                          frame->len == BRINGUP_FRAME_BYTES;
            if (!event.valid) {
                event.reason = BRINGUP_FRAME_REASON_INVALID_SIZE;
            }
#if CONFIG_BRINGUP_FAULT_INJECTION && CONFIG_BRINGUP_FAULT_SCENARIO_HOLD_FRAME
            if (fault_selected && event.valid && first_valid_seen && !delayed_once &&
                event.at_ms >= first_valid_ms &&
                event.at_ms - first_valid_ms >=
                    (uint64_t)CONFIG_BRINGUP_FAULT_AFTER_FIRST_VALID_S * 1000U) {
                delayed_once = true;
                ESP_LOGW("bringup", "FAULT_SCENARIO name=HOLD_FRAME worker=%u hold_ms=%u",
                         worker_index, CONFIG_BRINGUP_FAULT_HOLD_MS);
                vTaskDelay(pdMS_TO_TICKS(CONFIG_BRINGUP_FAULT_HOLD_MS));
            }
#endif
            esp_camera_fb_return(frame);
            if (atomic_load(&stop_requested)) {
                ESP_LOGI("bringup", "WORKER_EVENT fb_returned at_ms=%llu",
                         (unsigned long long)now_ms());
            }
        }
        if (atomic_load(&stop_requested)) {
            ESP_LOGI("bringup", "WORKER_EVENT late_frame_discarded at_ms=%llu",
                     (unsigned long long)now_ms());
            break;
        }
#if CONFIG_BRINGUP_FAULT_INJECTION
        bool initial_recovery_trigger = false;
#if CONFIG_BRINGUP_FAULT_SCENARIO_INIT_NO_FRAME
        initial_recovery_trigger = worker_index == 1U;
#endif
        const bool had_first_valid = first_valid_seen;
        if ((fault_selected || initial_recovery_trigger) && event.valid && !first_valid_seen) {
            first_valid_seen = true;
            first_valid_ms = event.at_ms;
        }
        const bool fault_due = (fault_selected || initial_recovery_trigger) && had_first_valid &&
            event.at_ms >= first_valid_ms &&
            event.at_ms - first_valid_ms >=
                (uint64_t)CONFIG_BRINGUP_FAULT_AFTER_FIRST_VALID_S * 1000U;
#if CONFIG_BRINGUP_FAULT_SCENARIO_SINGLE_INVALID || CONFIG_BRINGUP_FAULT_SCENARIO_THREE_INVALID
        if (fault_due && invalid_remaining > 0U && event.valid) {
            --invalid_remaining;
            event.valid = false;
            event.reason = BRINGUP_FRAME_REASON_INVALID_SIZE;
            ESP_LOGW("bringup", "FAULT_SCENARIO name=INVALID_FRAME worker=%u remaining=%u",
                     worker_index, invalid_remaining);
        }
#endif
#if CONFIG_BRINGUP_FAULT_SCENARIO_NO_FRAMES || CONFIG_BRINGUP_FAULT_SCENARIO_WORKER_DELAY || CONFIG_BRINGUP_FAULT_SCENARIO_RECOVERY_INIT || CONFIG_BRINGUP_FAULT_SCENARIO_INIT_NO_FRAME
        if (fault_due && event.valid && !delayed_once
#if CONFIG_BRINGUP_FAULT_SCENARIO_RECOVERY_INIT || CONFIG_BRINGUP_FAULT_SCENARIO_INIT_NO_FRAME
            && worker_index == 1U
#endif
        ) {
            delayed_once = true;
            ESP_LOGW("bringup", "FAULT_SCENARIO name=%s worker=%u delay_ms=%u",
#if CONFIG_BRINGUP_FAULT_SCENARIO_NO_FRAMES
                     "NO_FRAMES",
#elif CONFIG_BRINGUP_FAULT_SCENARIO_RECOVERY_INIT || CONFIG_BRINGUP_FAULT_SCENARIO_INIT_NO_FRAME
                     "RECOVERY_TRIGGER",
#else
                     "WORKER_DELAY",
#endif
                     worker_index, BRINGUP_FAULT_DELAY_MS);
            send_event(&event);
            vTaskDelay(pdMS_TO_TICKS(BRINGUP_FAULT_DELAY_MS));
            continue;
        }
#endif
        (void)fault_due;
        (void)invalid_remaining;
        (void)delayed_once;
#endif
        if (frame == NULL) {
            vTaskDelay(pdMS_TO_TICKS(BRINGUP_CAMERA_NULL_RETRY_MS));
        }
        send_event(&event);
    }
    worker_finish(BRINGUP_CAMERA_STOPPED, 0, true);
}

QueueHandle_t bringup_camera_start(unsigned attempt)
{
    if (attempt == 0U || atomic_exchange(&worker_active, true)) {
        return NULL;
    }
    if (camera_events == NULL) {
        camera_events = xQueueCreate(BRINGUP_CAMERA_EVENT_QUEUE_DEPTH,
                                     sizeof(bringup_camera_event_t));
        if (camera_events == NULL) {
            atomic_store(&worker_active, false);
            return NULL;
        }
    }
    atomic_store(&stop_requested, false);
    active_worker_index = ++worker_sequence;
    if (xTaskCreate(camera_worker, "camera_worker", BRINGUP_CAMERA_WORKER_STACK_BYTES,
                    (void *)(uintptr_t)attempt, BRINGUP_CAMERA_WORKER_PRIORITY,
                    NULL) != pdPASS) {
        atomic_store(&worker_active, false);
        --worker_sequence;
        return NULL;
    }
    return camera_events;
}

void bringup_camera_request_stop(void)
{
    atomic_store(&stop_requested, true);
    ESP_LOGI("bringup", "WORKER_EVENT stop_requested at_ms=%llu",
             (unsigned long long)now_ms());
}
