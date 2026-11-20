#include "bringup_camera.h"

#include <stdbool.h>

#include "esp_camera.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "board_pins.h"
#include "bringup_config.h"
#include "bringup_log.h"

static QueueHandle_t camera_events;

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static void send_event(const bringup_camera_event_t *event)
{
    xQueueSend(camera_events, event, portMAX_DELAY);
}

static void camera_worker(void *unused)
{
    (void)unused;
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
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    const esp_err_t result = esp_camera_init(&config);
    sensor_t *sensor = result == ESP_OK ? esp_camera_sensor_get() : NULL;
    if (result != ESP_OK || sensor == NULL || sensor->id.PID != OV3660_PID) {
        const int raw_error = result != ESP_OK ? result : (sensor == NULL ? -1 : sensor->id.PID);
        bringup_log_camera_init(1U, false, NULL, raw_error);
        if (result == ESP_OK) {
            esp_camera_deinit();
        }
        const bringup_camera_event_t failed = {
            .kind = BRINGUP_CAMERA_INIT_FAILED, .at_ms = now_ms(), .raw_error = raw_error,
        };
        send_event(&failed);
        vTaskDelete(NULL);
        return;
    }

    bringup_log_camera_init(1U, true, "OV3660", 0);
    const bringup_camera_event_t ready = { .kind = BRINGUP_CAMERA_READY, .at_ms = now_ms() };
    send_event(&ready);

    for (;;) {
        camera_fb_t *frame = esp_camera_fb_get();
        bringup_camera_event_t event = { .kind = BRINGUP_CAMERA_FRAME, .at_ms = now_ms() };
        if (frame != NULL) {
            event.width = frame->width;
            event.height = frame->height;
            event.bytes = frame->len;
            event.valid = frame->buf != NULL && frame->width == BRINGUP_FRAME_WIDTH &&
                          frame->height == BRINGUP_FRAME_HEIGHT &&
                          frame->len == BRINGUP_FRAME_BYTES;
            esp_camera_fb_return(frame);
        }
        send_event(&event);
    }
}

QueueHandle_t bringup_camera_start(void)
{
    if (camera_events != NULL) {
        return NULL;
    }
    camera_events = xQueueCreate(BRINGUP_CAMERA_EVENT_QUEUE_DEPTH,
                                 sizeof(bringup_camera_event_t));
    if (camera_events == NULL) {
        return NULL;
    }
    if (xTaskCreate(camera_worker, "camera_worker", BRINGUP_CAMERA_WORKER_STACK_BYTES,
                    NULL, BRINGUP_CAMERA_WORKER_PRIORITY, NULL) != pdPASS) {
        vQueueDelete(camera_events);
        camera_events = NULL;
        return NULL;
    }
    return camera_events;
}
