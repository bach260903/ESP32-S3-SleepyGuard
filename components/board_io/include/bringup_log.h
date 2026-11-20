#ifndef BRINGUP_LOG_H
#define BRINGUP_LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void bringup_log_boot_info(bool psram_initialized, size_t psram_raw_bytes,
                           size_t psram_expected_bytes, size_t psram_heap_free_bytes,
                           uint32_t flash_physical_bytes, uint32_t flash_expected_bytes);
void bringup_log_psram_test(size_t test_bytes, const char *patterns, bool passed,
                            const char *error_code);
void bringup_log_camera_init(unsigned attempt, bool success, const char *sensor,
                             int raw_error);
void bringup_log_frame_report(unsigned width, unsigned height, uint32_t valid,
                              uint32_t invalid, uint64_t elapsed_ms, double fps);
void bringup_log_frame_unavailable(const char *stage, const char *reason,
                                   int raw_error);
void bringup_log_recovery(const char *stage, unsigned remaining, const char *reason);
void bringup_log_fault(const char *code, const char *reason, unsigned remaining);

#endif
