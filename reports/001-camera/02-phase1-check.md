# Phase 1 checkpoint — 2026-11-20 (Asia/Bangkok)

- T001: ESP-IDF tag v5.5.2, target esp32s3; dependency lock resolves
  `espressif/esp32-camera` 2.1.9 and `espressif/esp_jpeg` 1.3.2. Minimal build
  passed; full output in `01-build.txt`.
- T002: `tools/capture_serial.py` runs with ESP-IDF v5.5 Python/pyserial 3.5;
  help and invalid duration checked. It releases DTR and resets via RTS on open
  so CH343 boots the app rather than remaining in DOWNLOAD mode.
- T003: flashed through CH343 COM5, esptool verified hashes. BOOT_INFO in
  `01-psram-flash.log`: initialized=1, raw PSRAM=8,388,608 bytes, expected
  8,388,608; free PSRAM heap=8,386,196 bytes; physical flash=16,777,216
  bytes, expected 16,777,216. `01-sdkconfig.txt` records the generated config.
- T004: flashed through CH343 COM5, esptool verified hashes. New firmware log
  `02-camera-probe.log` identifies OV3660 PID 0x3660. Three returned RGB565
  frames are each 320×240, 153,600 bytes, valid=1. `02-sdkconfig.txt` records
  the generated config. No `CAM_INIT_FAILED`, `FRAME_UNAVAILABLE` or panic in
  the successful probe log.
- Generated config: flash 16 MB, octal PSRAM, ECC and PSRAM XIP off, UART
  console primary, USB Serial/JTAG console secondary off.

Phase 1 is complete. Phase 2 logic, PSRAM pattern test, continuous FPS reports,
recovery and fault injection have not been implemented yet.
