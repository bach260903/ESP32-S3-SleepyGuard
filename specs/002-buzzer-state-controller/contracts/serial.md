# Serial contract: feature 002 bench mode

Console remains 115200 baud. Text lines are ASCII, terminated by LF or CRLF. This interface is for bench injection only; it is not a network or production eye-detection interface.

## Input commands

One command per line, case-sensitive:

| Command | Observation | Expected response |
| --- | --- | --- |
| `EYE OPEN` | OPEN | `MOCK_INPUT result=accepted kind=OPEN` |
| `EYE CLOSED` | CLOSED | `MOCK_INPUT result=accepted kind=CLOSED` |
| `FACE LOST` | NO_FACE | `MOCK_INPUT result=accepted kind=NO_FACE` |
| `ALERT ACK` | ACK | `MOCK_INPUT result=accepted kind=ACK` |
| `CAMERA FAULT` | CAMERA_FAULT, bench only | `MOCK_INPUT result=accepted kind=CAMERA_FAULT` |
| `CAMERA RECOVERED` | only if a valid camera frame has arrived | `MOCK_INPUT result=accepted kind=CAMERA_RECOVERED` or a rejection reason |

Anything else: `MOCK_INPUT result=rejected reason=unknown_command`. Rejected input never changes state. A mock eye command cannot clear a real camera fault. Input processing must be bounded and must not block the camera supervisor.

## Output lines

- `ALERT_STATE from=<phase> to=<phase> reason=<code> at_ms=<ms> source=<source>` on each transition.
- `ALERT_OUTPUT phase=<phase> buzzer=<0|1> led=<0|1> muted=<0|1> at_ms=<ms>` when either output changes; values are logical ON/OFF before polarity mapping.
- `ALERT_FAULT code=<code> last_cause=<cause> at_ms=<ms>` when camera fault starts and every 5000 ms while it persists. After ACK, this line and LED continue, even though buzzer stays off.
- `PIN_PROBE gpio=<n> level=<0|1> elapsed_ms=<ms> candidate=<LED|BUZZER>` for each bounded pin test. The paired check report records observed light/sound, supply voltage, polarity, and whether the buzzer driver is buffered.

Existing `BOOT_INFO`, `CAMERA_INIT`, `FRAME_REPORT`, `RECOVERY` and `FAULT` lines from feature 001 retain their existing meanings. Boot ROM timestamps and `at_ms` have distinct origins as documented in feature 001.

No image bytes or face data are transmitted in this contract.
