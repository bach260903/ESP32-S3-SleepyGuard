#include "bringup_logic.h"

#include <stddef.h>

void bringup_measurement_start(bringup_measurement_t *window, uint64_t now_ms,
                               uint16_t width, uint16_t height)
{
    if (window == NULL) {
        return;
    }
    *window = (bringup_measurement_t){
        .started_ms = now_ms,
        .width = width,
        .height = height,
    };
}

void bringup_measurement_record(bringup_measurement_t *window,
                                const bringup_frame_observation_t *frame)
{
    if (window == NULL || frame == NULL) {
        return;
    }
    if (frame->valid && frame->width == window->width &&
        frame->height == window->height) {
        ++window->valid;
    } else {
        ++window->invalid;
    }
}

bool bringup_measurement_due(const bringup_measurement_t *window,
                             uint64_t now_ms, uint32_t period_ms)
{
    return window != NULL && now_ms >= window->started_ms &&
           now_ms - window->started_ms >= period_ms;
}

bool bringup_measurement_finish(bringup_measurement_t *window,
                                uint64_t now_ms, bringup_measurement_t *report)
{
    if (window == NULL || report == NULL || now_ms <= window->started_ms ||
        window == report) {
        return false;
    }
    *report = *window;
    report->elapsed_ms = now_ms - window->started_ms;
    report->fps = (double)report->valid * 1000.0 / (double)report->elapsed_ms;
    bringup_measurement_start(window, now_ms, report->width, report->height);
    return true;
}
