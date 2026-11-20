# US1 board checkpoint — 2026-11-20 (Asia/Bangkok)

- Build and flash: `04-us1-build.txt` and `04-us1-flash.txt`; flash hashes verified
  on CH343 COM5. Generated configuration saved as `04-sdkconfig.txt`.
- Observation: `04-us1-stream-125s.log`, captured without interaction for 125
  seconds after the capture tool reset the board.
- Startup order: `BOOT_INFO` (PSRAM raw 8,388,608 bytes; physical flash
  16,777,216 bytes) → `PSRAM_TEST` PASS (153,600 bytes; address and inverse
  patterns) → `CAMERA_INIT` success (OV3660).
- Stream: **24** `FRAME_REPORT` lines in the 125-second capture. Every report
  shows 320×240, `valid > 0`, `fps > 0`, and `invalid=0`. In total, 334 valid
  frames and zero invalid frames were reported. FPS ranged from 2.36 to 2.78;
  after the first interval it stayed around 2.78. This is a measured result
  with one framebuffer, not a minimum performance target.
- No `FRAME_UNAVAILABLE`, `FAULT`, `CAM_INIT_FAILED`, or warning/error from
  `cam_hal` appears in the capture.
- SC-001 and SC-002: PASS on this board. US2/US3 fault and recovery behavior
  remains to be implemented and validated separately.
