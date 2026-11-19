#include <inttypes.h>
#include <stdbool.h>

#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_camera.h"
#include "board_pins.h"

static const char *TAG = "bringup";

void app_main(void)
{
    const bool psram_initialized = esp_psram_is_initialized();
    const size_t psram_size_raw_bytes = esp_psram_get_size();
    const size_t psram_heap_free_bytes = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    uint32_t flash_physical_bytes = 0;
    const esp_err_t flash_result = esp_flash_get_physical_size(
        esp_flash_default_chip, &flash_physical_bytes);

    ESP_LOGI(TAG,
             "BOOT_INFO psram_initialized=%d psram_size_raw_bytes=%zu "
             "psram_expected_bytes=%u psram_heap_free_bytes=%zu "
             "flash_physical=%" PRIu32 " flash_expected=%u",
             psram_initialized, psram_size_raw_bytes, 8U * 1024U * 1024U,
             psram_heap_free_bytes, flash_physical_bytes, 16U * 1024U * 1024U);
    if (flash_result != ESP_OK) {
        ESP_LOGE(TAG, "BOOT_INFO flash_error=%s", esp_err_to_name(flash_result));
    }

    if (!psram_initialized) {
        ESP_LOGE(TAG, "PSRAM_UNAVAILABLE stage=camera_probe");
        return;
    }

    camera_config_t camera = {
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
        .xclk_freq_hz = 10000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 12,
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    ESP_LOGI(TAG, "CAMERA_INIT stage=probe frame_size=QVGA pixel_format=RGB565 fb_count=1");
    const esp_err_t camera_result = esp_camera_init(&camera);
    if (camera_result != ESP_OK) {
        ESP_LOGE(TAG, "CAM_INIT_FAILED stage=probe raw_error=0x%x name=%s",
                 camera_result, esp_err_to_name(camera_result));
        return;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == NULL || sensor->id.PID != OV3660_PID) {
        ESP_LOGE(TAG, "CAM_INIT_FAILED stage=sensor_check pid=0x%x expected=0x%x",
                 sensor == NULL ? 0 : sensor->id.PID, OV3660_PID);
        esp_camera_deinit();
        return;
    }
    ESP_LOGI(TAG, "CAMERA_INIT result=success sensor=OV3660 pid=0x%x", sensor->id.PID);

    for (unsigned attempt = 1; attempt <= 3; ++attempt) {
        camera_fb_t *frame = esp_camera_fb_get();
        if (frame == NULL) {
            ESP_LOGE(TAG, "FRAME_UNAVAILABLE stage=probe attempt=%u reason=fb_get_null", attempt);
            continue;
        }
        const bool valid = frame->buf != NULL && frame->len > 0 &&
                           frame->width == 320 && frame->height == 240;
        ESP_LOGI(TAG, "FRAME_PROBE attempt=%u width=%zu height=%zu bytes=%zu valid=%d",
                 attempt, frame->width, frame->height, frame->len, valid);
        esp_camera_fb_return(frame);
    }
    esp_camera_deinit();
}
