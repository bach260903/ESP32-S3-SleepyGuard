# US2 startup faults — board checkpoint (2026-11-20)

## PC test: red then green

- `05-us2-host-red.txt`: host test compiled and ran, then failed at the first
  startup attempt before INIT/FAULT transitions were implemented.
- `05-us2-host-green.txt`: the same test passed after implementation. It covers
  exactly three attempts, terminal FAULT, immediate and 5,000 ms periodic
  reports, no further camera attempts, PSRAM fault without camera init, and a
  successful second camera attempt. The logic accepts caller-provided `now_ms`
  and has no ESP-IDF includes.

## Board scenarios

Each row has its own `05-us2-<scenario>-build.txt`, `-flash.txt`,
`-sdkconfig.txt`, and `.log`. The generated sdkconfig selected exactly one
Kconfig scenario. `05-us2-validation.txt` contains the automated checks.

| Scenario | Final FAULT code and reason | CAMERA_INIT attempts | ROM boots | FAULT observation | Report gap |
|---|---|---:|---:|---:|---:|
| PSRAM unavailable | `PSRAM_UNAVAILABLE`, `injected_unavailable` | 0 | 1 | 14 lines, 65,000 ms | 5,000 ms |
| PSRAM capacity mismatch | `PSRAM_CONFIG_MISMATCH`, `injected_capacity_mismatch` | 0 | 1 | 14 lines, 65,000 ms | 5,000 ms |
| PSRAM allocation failure | `PSRAM_TEST_FAILED`, `injected_allocation_failed` | 0 | 1 | 14 lines, 65,000 ms | 5,000 ms |
| PSRAM readback mismatch | `PSRAM_TEST_FAILED`, `injected_readback_mismatch` | 0 | 1 | 14 lines, 65,000 ms | 5,000 ms |
| Camera init failure | `RECOVERY_EXHAUSTED`, `camera_init_exhausted` | 3 (attempts 1, 2, 3) | 1 | 14 lines, 65,010 ms | 5,000–5,010 ms |

The PSRAM scenarios log `PSRAM_TEST result=FAIL` before entering FAULT and
never log `CAMERA_INIT`. The camera scenario logs `PSRAM_TEST result=PASS`,
three `CAM_INIT_FAILED` events with raw error `0xffffffff`, then terminal
FAULT. All scenarios report the fault code and reason immediately on entering
FAULT and continue every five seconds without another ROM boot.

The first camera scenario build failed because its new log call was missing
`esp_log.h`. The original error is preserved in
`05-us2-camera-init-build-error.txt`; the header was added and the same
scenario built, flashed and passed.

## Normal build restored

`05-us2-normal-sdkconfig.txt` has `CONFIG_BRINGUP_FAULT_INJECTION` disabled.
`05-us2-normal-check.txt` confirms the app binary contains neither
`FAULT_SCENARIO` nor `injected_`. The normal firmware was flashed again;
`05-us2-normal-12s.log` shows one boot, PSRAM PASS, OV3660 camera init success,
two valid 320×240 reports, and no fault. The board is left on this normal
build. US3 recovery and blocked `fb_get` remain for the next phase.
