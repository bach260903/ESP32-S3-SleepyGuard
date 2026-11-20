# US3 held-frame checkpoint (2026-11-20)

- Build: `06-us3-held-frame-build.txt`; flash: `06-us3-held-frame-flash.txt` on CH343 COM5; generated configuration: `06-us3-held-frame-sdkconfig.txt`.
- BOOT_INFO: PSRAM raw 8,388,608 bytes; flash 16,777,216 bytes. PSRAM_TEST passed before CAMERA_INIT. One boot ROM sequence; no reboot in the 32-second capture.
- Worker 1 captured one valid frame, then held the next real framebuffer for 6,000 ms. Measured `fb_get` calls were 282 ms and 68 ms; the six-second hold was a deliberate delay after `fb_get`, not driver blocking time.
- Ordered log markers: `RECOVERY stage=start` → `WORKER_EVENT stop_requested` → `RECOVERY_WAIT` → `fb_returned` → `late_frame_discarded` → `deinit_done result=ESP_OK` → `terminal=STOPPED` → worker 2 `CAMERA_INIT` → `RECOVERY stage=success`.
- Worker 2 returned valid frames after reinit: 15.00, 14.81, 14.81, 14.81 FPS in consecutive five-second reports, each with `invalid=0`.
- The worker stopped about one second after the supervisor requested it, so this capture only exercised the immediate RECOVERY_WAIT report. The five-second repeat and 15-second stop timeout have host boundary tests; a board capture with a longer blocked worker is still needed to observe repeated wait reports.
- The normal firmware was rebuilt and flashed after the scenario. `CONFIG_BRINGUP_FAULT_INJECTION` is off, fault scenario strings are absent from the application binary, and `06-us3-normal-12s.log` shows valid camera frames with `invalid=0` and no recovery/fault line.

Log: `06-us3-held-frame.log`.
