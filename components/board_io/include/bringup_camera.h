#ifndef BRINGUP_CAMERA_H
#define BRINGUP_CAMERA_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    BRINGUP_CAMERA_READY,
    BRINGUP_CAMERA_INIT_FAILED,
    BRINGUP_CAMERA_FRAME,
    BRINGUP_CAMERA_STOPPED,
} bringup_camera_event_kind_t;

typedef enum {
    BRINGUP_FRAME_REASON_NONE,
    BRINGUP_FRAME_REASON_FB_GET_NULL,
    BRINGUP_FRAME_REASON_INVALID_SIZE,
} bringup_frame_reason_t;

typedef struct {
    bringup_camera_event_kind_t kind;
    uint64_t at_ms;
    int raw_error;
    unsigned width;
    unsigned height;
    unsigned bytes;
    bool valid;
    bringup_frame_reason_t reason;
} bringup_camera_event_t;

/* The worker is the only caller of esp_camera_init/fb_get/fb_return/deinit. */
QueueHandle_t bringup_camera_start(unsigned attempt);
void bringup_camera_request_stop(void);

#endif
