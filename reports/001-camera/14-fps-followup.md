# OV3660 QVGA RGB565 FPS follow-up — 2026-11-21

Board: ESP32-S3 N16R8, OV3660, ESP-IDF 5.5.2, esp32-camera 2.1.9.
Both experiments used two framebuffers and 80 MHz octal PSRAM. Each was
built, flashed to the board on COM5, and captured for 45 seconds. The
first five-second report includes startup time; the remaining reports
measure steady capture.

| Requested XCLK | CPU | PSRAM DMA | Driver-calculated PCLK (not measured) | Steady FPS | Invalid frames | Log |
| --- | --- | --- | --- | --- | --- | --- |
| 24 MHz | 160 MHz | off | 12 MHz | 14.81 | 0 | `11-xclk24-stream-125s.log` |
| 24 MHz | 240 MHz | on | 12 MHz | 14.81 | 0 | `12-psram-dma-cpu240-45s.log` |
| 26 MHz | 240 MHz | on | 13 MHz | 14.81 | 0 | `13-xclk26-dma-cpu240-45s.log` |

The two experiments produced seven steady reports each at 14.81 FPS,
with no frame errors or recovery events. PSRAM DMA initialized and ran,
but did not improve this capture-only workload in these runs. CPU and DMA
changed together, so their individual effects were not isolated.
The project returned to the previously confirmed 24 MHz XCLK request,
160 MHz CPU and non-DMA path. This does not establish the sensor's
absolute maximum FPS across different resolutions, pixel formats, or
sensor timing registers. Future eye detection may also change the
throughput profile.

## Correction — 2026-11-22

The earlier interpretation of the 24-to-26 MHz experiment was wrong.
`target/esp32s3/ll_cam.c` in the pinned driver sets fractional divider A/B
to zero and uses integer `160000000 / config->xclk_freq_hz` for the divider.
Both requests select divider 6. ESP-IDF's S3 `cam_ll.h` identifies source
selection 3 as PLL160M. Both configurations therefore generate the same
nominal XCLK, approximately 26.667 MHz; this is inferred from configuration,
not measured with an oscilloscope. The sensor's PLL log calculates from
the requested XCLK and cannot prove that PCLK physically changed.

The identical FPS in those two runs is thus not evidence of a sensor FPS
ceiling. The raw UART captures remain unchanged. The reported 14.81 is
75 accepted frame events / 5.065 seconds, rounded to two decimals. Validity
checks dimensions and byte count, not image content or unique image data.
Firmware now logs the clock source and divider registers after init so
subsequent comparisons can check whether clock hardware settings differ.
The baseline was restored and captured in `15-restored-baseline-17s.log`.
