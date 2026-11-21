# US3 board validation (2026-11-21)

All scenarios below used separate ESP-IDF v5.5.2 / esp32-camera 2.1.9 builds,
flash logs, generated sdkconfig snapshots, and serial logs on CH343 COM5.
`06-us3-host-tests.txt` records passing smoke, startup, recovery, and deadline
decision tests. Every serial capture in the table has one boot ROM sequence.

| Scenario | Evidence | Result |
|---|---|---|
| Held framebuffer 6 s | `06-us3-held-frame.log`, `06-us3-held-frame-check.md` | Frame returned before deinit, STOPPED before reinit, real frames resume. |
| Held framebuffer 17 s | `06-us3-hold-long.log` | RECOVERY_WAIT at 0, 5.010, 10.010 s; STOPPED before reinit; 15.00/14.81 FPS after recovery. |
| Held framebuffer 23 s | `06-us3-hold-timeout.log` | WORKER_STOP_TIMEOUT at 15.010 s after stop request; late STOPPED, then continued five-second FAULT reports; no reinit/reboot. |
| One invalid frame | `06-us3-single-invalid.log` | One FRAME_UNAVAILABLE, first FRAME_REPORT invalid=1, no recovery. |
| Three invalid frames | `06-us3-three-invalid.log` | Three FRAME_UNAVAILABLE, interrupted FRAME_REPORT valid=1 invalid=3 before recovery, attempt starts with remaining=2; real frames and about 15 FPS after reinit. |
| No frames on one worker | `06-us3-no-frames-once.log` | Five-second frame-loss deadline, attempt remaining=2, real frames and about 15 FPS after reinit. |
| Worker delay after frame return | `06-us3-worker-delay.log` | Deliberate 6-second worker delay starts after the real frame was returned; frame-loss recovery succeeds with real frames and about 15 FPS. This is not evidence of a 6-second `fb_get`. |
| All recovery init attempts fail | `06-us3-recovery-init-all.log` | Attempt starts at remaining=2,1,0; matching failure results; RECOVERY_EXHAUSTED, FAULT reports every five seconds for more than 60 s, one boot ROM sequence. |
| Recovery init succeeds but no first frame | `06-us3-init-no-frame.log` | Worker 2 READY at about 7.61 s, first-frame timeout at 12.62 s, one failed result with remaining=2; worker 3 succeeds with remaining=1. |
| Repeated failure before stable 60 s | `06-us3-repeat-before-60.log` | Successful recovery at remaining=2,1,0; next failure exhausts without budget reset. |
| Repeated failure after stable 60 s | `06-us3-repeat-after-60.log` | First recovery succeeds with remaining=2 at 67.074 s. Budget resets to 3 at 127.114 s, 60.040 s later. Next loss starts at 132.254 s and next attempt has remaining=2. |

The repeated-failure captures account for every injected invalid frame in
FRAME_REPORT. In `repeat-before-60`, each three-frame burst is reported as
3, or as 1 in the regular five-second report plus 2 in the partial report
before recovery. No partial-window counts appear in the first report after
recovery.

## Heap observations

At each `RECOVERY stage=start`, firmware logs PSRAM and internal free heap.
At every successful driver init, including a driver init followed by injected
failure, it logs both values again. In `recovery-init-all`, PSRAM free was
8,077,744 bytes and internal free was 322,924 bytes after each of workers 2,
3 and 4 initialized. In `repeat-before-60`, the same two values appeared
after each of workers 2, 3 and 4 initialized and at subsequent recovery starts.
Worker 1 had 220 more internal free bytes than later workers; the later
values were stable rather than declining per cycle. This is evidence against
a monotonic leak over the three observed cycles, not a long-duration leak test.

### Extended run attempted on 2026-11-21

`06-us3-leak-interrupted-21min.log` used `THREE_INVALID`, worker_index=0 and
after_first_valid_s=65. Its first 19 recovery starts and 19 successful
recoveries had one boot ROM sequence. At every recovery start, PSRAM free was
8,077,744 bytes and internal free was 322,916 bytes (spread 0 for both).
Serial output stopped after worker 20's successful recovery at about 20.9
minutes. COM5 remained listed, but reopening CH343 failed twice with Windows
error 31 (device not functioning); restarting the device via PnP was denied
by Windows. The requested 30-cycle conclusion is therefore **not established**.
The interrupted capture does not distinguish a firmware stall from a
USB-UART/driver connection failure. Reconnect CH343 and repeat the full
30–40-minute run before treating extended operation as validated.

The later `06-us3-fault-fields.log` confirms the Serial format update on
board: `CAMERA_INIT phase=boot worker=1` differs from
`phase=recovery worker=2/3/4`; every repeated `RECOVERY_EXHAUSTED` FAULT line
includes `last_cause=camera_init_failed`. The capture has one boot ROM
sequence and its own build, flash, and sdkconfig snapshot.

## Log interpretation

The held-frame captures distinguish measured `fb_get` durations (67–278 ms)
from deliberate 6/17/23-second holds after `fb_get`. In the 17-second capture,
the frame-loss decision came about 5.1 seconds after the last valid frame.
The recovery budget decremented when the new worker started, not when frame
loss was first latched. No FRAME_REPORT appeared between the recovery latch
and the first valid frame from the new worker. Its first report used a fresh
roughly five-second window. For three invalid frames, the previous partial
window was emitted immediately before the recovery latch so invalid=3 remains
visible without mixing pre-recovery counts into the next window.
