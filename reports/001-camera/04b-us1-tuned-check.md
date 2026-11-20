# US1 tuned camera checkpoint — 2026-11-20

Selected configuration: OV3660 QVGA RGB565, XCLK 24 MHz, two framebuffers,
`CAMERA_GRAB_WHEN_EMPTY`, octal PSRAM 80 MHz. The previous `04-` files remain
untouched as the 10 MHz, one framebuffer, 40 MHz PSRAM baseline.

`04b-us1-tuned-125s.log` captured 125 seconds after reset on CH343 COM5:
24 `FRAME_REPORT` lines, 1,796 valid frames, zero invalid frames, and FPS
14.17 in the first window then 14.81 in every steady window. Startup order
was BOOT_INFO → PSRAM_TEST PASS → CAMERA_INIT success. No
`FRAME_UNAVAILABLE`, `FAULT`, `CAM_INIT_FAILED`, or `cam_hal` warning/error
appeared. `04b-sdkconfig.txt` confirms flash 16 MB, octal PSRAM 80 MHz and
UART console. Build and flash output are in `04b-us1-tuned-build.txt` and
`04b-us1-tuned-flash.txt`.

A separate 42-second trial with three framebuffers (`04c-fb3-42s.log`)
produced eight reports, 597 valid frames, zero invalid frames and 14.80–14.81
steady FPS. It showed no measurable speed gain over two buffers. The source
and board were restored to two buffers; `04d-restore-fb2-12s.log` confirms
two PSRAM framebuffer allocations and 14.81 FPS after the first window.

The 14.81 FPS figure measures capture and metadata checks only. It does not
include image inference. Image content was not visually inspected.
