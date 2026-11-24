# Research: alert controller

## R1 — Pin and buzzer identity

- **Decision**: Mark buzzer GPIO, LED GPIO, polarity and output type unverified until T001 logs each candidate toggle and a human records observed light/sound. Prior GPIO2/GPIO3 bench code offers candidates only.
- **Rationale**: A Serial line proves software toggled a register, not the connected device. An active three-pin buffered module can use on/off; a passive buzzer needs an audio-frequency waveform and a verified driver stage. A bare two-wire speaker must not be driven directly by GPIO under D-013.
- **Alternatives considered**: copy labels/photos, infer from earlier probe log, or assume every sounding two-wire device is an active module. Each can pick the wrong pin or overload GPIO.

## R2 — Clock and event model

- **Decision**: Give a pure state function timestamped observations in monotonic milliseconds; keep GPIO/LEDC and Serial I/O outside it. A periodic output tick computes levels without waiting for a beep to finish.
- **Rationale**: PC tests can assert exact boundaries, and camera events keep flowing during all sound patterns. A separate mute latch resets only when a new episode starts.
- **Alternatives considered**: sleep inside beep functions or poll Serial before servicing camera events; both can delay fault handling.

## R3 — Pattern meaning

- **Decision**: Use three different temporal patterns: NGHI NGỜ 100 ms on every 1000 ms, BÁO ĐỘNG 200 ms on / 200 ms off, LỖI CAMERA three 100 ms pulses within a 5-second period. LED uses steady TỈNH, slow single flash KHÔNG THẤY MẶT, sparse flash NGHI NGỜ, fast flash BÁO ĐỘNG, triple flash LỖI CAMERA. Output polarity is filled after T001.
- **Rationale**: A fixed-volume active buzzer cannot become physically louder by changing the GPIO level. Faster denser beeps communicate urgency. The error triple is distinct and repeats through permanent FAULT.
- **Alternatives considered**: change PWM duty for volume on an unknown module; behavior depends on hardware and would be misleading.

## R4 — Camera integration

- **Decision**: Feed the alert controller camera health from existing startup/recovery supervisor. Enter camera fault at the fault latch or startup fault, retain code and last cause, and return to TỈNH only after a valid recovered frame. On fault, clear closure timing; late worker events cannot restore alert health.
- **Rationale**: The measured common gap is close to six seconds. Waiting until final exhaustion would leave that gap silent. The supervisor already owns the authoritative recovery state.
- **Alternatives considered**: derive health from FPS reports or last Serial line; five-second report windows are too slow.

## R5 — Test data and scope

- **Decision**: Use textual mock observations and metadata logs only. Face video used later for AI evaluation belongs under ignored `testdata/`, never in Git.
- **Rationale**: Constitution V bans recording or transmitting face images in firmware; the public repository must not contain private face samples.
- **Alternatives considered**: record raw camera frames to SD or logs; excluded.
