# Feature 001 final board check (2026-11-21)

## Build and configuration

- ESP-IDF v5.5.2, target esp32s3, `espressif/esp32-camera` 2.1.9.
- `07-production-build.txt` is a clean build. `07-sdkconfig.txt` and
  `07-final-sdkconfig.txt` both have `CONFIG_BRINGUP_FAULT_INJECTION` off.
- `07-production-binary-check.txt` confirms `FAULT_SCENARIO`,
  `INVALID_FRAME`, `RECOVERY_TRIGGER`, and `WORKER_DELAY` are absent from both
  `.elf` and `.map`.
- `07-final-flash.txt` records a successful COM5 flash with hashes verified.
  The preceding port failure is preserved separately in
  `07-final-flash-attempt.txt` and `07-port-block.md`; it wrote no firmware.

## Serial acceptance

`07-final-125s.log` was captured through USB-UART/CH343 at 115200 baud.
`07-final-log-check.txt` records the automated checks, and the log was also
read for ordering and scope:

- Exactly one boot ROM sequence; no reboot.
- `BOOT_INFO`: PSRAM raw and expected 8,388,608 bytes; flash physical and
  expected 16,777,216 bytes.
- `PSRAM_TEST test_bytes=153600 patterns=address,inverse result=PASS` before
  `CAMERA_INIT phase=boot worker=1 attempt=1 result=success sensor=OV3660`.
- 24 FRAME_REPORT lines in 125 seconds, each 320×240 with `valid>0`,
  `invalid=0`, and FPS>0. Total valid=1,796, invalid=0; FPS range
  14.19–14.81. Reports were spaced about five seconds apart.
- No warning/error log lines, FRAME_UNAVAILABLE, RECOVERY, FAULT, or
  FAULT_SCENARIO lines. The firmware source has no SD, network or buzzer
  call paths; Serial contains metadata only, without image data.

## Requirement evidence

- FR-001/002/003/012/013 and SC-001/002: this final normal capture.
- FR-004/005/014 and SC-003/004: US2 fault captures in `05-us2-check.md`.
- FR-006/007/009/010/011 and SC-005/006/007: US3 host and board captures in
  `06-us3-check.md`, including the deadline, budget, late STOPPED and
  first-frame timeout cases.
- FR-008: source and build scope review; no recognition, buzzer, image
  storage, SD or network functionality in feature 001.

The optional extended leak check reached 19 recovery cycles with stable
heaps before CH343 stopped functioning; it did not establish a 30-cycle
result. Details and limits are in `06-us3-check.md`.

The final documentation was checked against D-001 (board and pins), D-003
(pinned IDF/component), D-004 (measured camera FPS, excluding inference),
D-007 (Serial-only fault reporting for feature 001 and future buzzer duty),
and constitution v2.0.0. This is bench diagnostic firmware, not a validated
on-road drowsiness warning device.
