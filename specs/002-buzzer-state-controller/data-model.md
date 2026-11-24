# Data model: alert controller

All time values use monotonic milliseconds. The logic component owns state only; hardware mappings are verified separately.

## AlertConfig

| Field | Type / constraint | Meaning |
| --- | --- | --- |
| `AlertConfig.suspect_ms` | positive integer; default 750; strictly less than `alarm_ms` | Continuous closed interval before NGHI NGỜ |
| `AlertConfig.alarm_ms` | positive integer; default 1500; greater than `suspect_ms` | Continuous closed interval before BÁO ĐỘNG |
| `AlertConfig.fault_repeat_ms` | positive integer; exactly 5000 in this feature | Period between camera fault patterns and status logs |
| `AlertConfig.tick_ms` | positive integer; at most 50 | Maximum scheduled output update interval |

The one central application configuration is the canonical source of these values. Tests may override them in memory.

## Observation

| Field | Type / constraint | Meaning |
| --- | --- | --- |
| `Observation.kind` | one of OPEN, CLOSED, NO_FACE, CAMERA_FAULT, CAMERA_RECOVERED, ACK | New input |
| `Observation.at_ms` | monotonic unsigned integer; must not precede the last accepted event time | Time observed |
| `Observation.source` | one of MOCK_SERIAL, CAMERA, BUTTON | Origin; camera health cannot be cleared by a mock eye event |

An invalid or backward event is rejected without changing state.

## AlertState

| Field | Type / constraint | Meaning |
| --- | --- | --- |
| `AlertState.phase` | exactly one of AWAKE, SUSPECT, ALARM, CAMERA_FAULT, NO_FACE | Current visible state |
| `AlertState.closed_since_ms` | nullable unsigned integer; set only during an uninterrupted CLOSED episode | Closure start |
| `AlertState.muted_episode` | boolean; reset on OPEN, NO_FACE, or a new camera fault episode | ACK latch |
| `AlertState.entered_ms` | unsigned integer; nondecreasing | Phase entry time |
| `AlertState.fault_code` | nullable nonempty code when phase is CAMERA_FAULT | Last camera fault code |
| `AlertState.last_cause` | nullable nonempty cause when phase is CAMERA_FAULT | Latest diagnostic cause |

### Transitions

| Current | Event / deadline | Next | Effect |
| --- | --- | --- | --- |
| AWAKE/NO_FACE | CLOSED | AWAKE | Start new closure interval |
| AWAKE | CLOSED duration ≥ suspect_ms | SUSPECT | Sparse warning |
| SUSPECT | CLOSED duration ≥ alarm_ms | ALARM | Dense alarm |
| Any nonfault | OPEN | AWAKE | Clear closure and mute |
| Any nonfault | NO_FACE | NO_FACE | Clear closure and mute |
| Any | CAMERA_FAULT | CAMERA_FAULT | Clear closure, keep code/cause, start error pattern |
| CAMERA_FAULT | CAMERA_RECOVERED plus valid frame | AWAKE | Clear error and old eye state |
| SUSPECT/ALARM/CAMERA_FAULT | ACK | same phase | Mute current audible episode; LED/log continue |

Repeated CLOSED events within one uninterrupted episode retain the original start timestamp. A mock OPEN cannot clear CAMERA_FAULT.

## OutputPattern

| Field | Type / constraint | Meaning |
| --- | --- | --- |
| `OutputPattern.buzzer_on` | boolean; false if muted or state is AWAKE/NO_FACE | Desired buzzer electrical state |
| `OutputPattern.led_on` | boolean | Desired LED electrical state before verified polarity mapping |
| `OutputPattern.next_due_ms` | unsigned integer; no more than 50 ms after current tick | Next update deadline |

Pattern identity derives from `AlertState.phase`; no blocking sleep in output calculation.
