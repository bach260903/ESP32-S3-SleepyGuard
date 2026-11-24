# Full-field QVGA RGB565 optimization

## Final user choice — 2026-11-24

The user selected approximately 30 FPS for practical use. The application
now uses `full_sys53_p17`: original full-field 2x binning, increments 0x31,
HTS 2300, VTS 783, PLL multiplier 16, predivider code 2 (/2), system divider
1, root doubling off, seld5 code 0, manual PCLK divider 6. With the actual
nominal XCLK 160/6 MHz, SYSCLK is 53.333 MHz and PCLK is 17.778 MHz.
Benchmark results were 29.615 FPS. CPU 240 MHz, PSRAM 80 MHz, PSRAM DMA and
two RGB565 QVGA buffers are retained. The 85 FPS candidate below is historical
experimental evidence, not the selected application preset.

The final regular application was built and flashed on COM4 with all bench,
preview, wiring and fault-injection modes disabled. The 125-second capture
(`28-practical30-production-125s.log`) passed `tools/check_final_log.py`:
one boot sequence, 24 healthy reports, 3,574 valid frames, zero invalid
frames, no warnings, errors, fault scenarios or recovery events. The first
window includes startup and measures 29.25 FPS; steady windows measure
29.61–29.62 FPS. The checker output is saved as
`28-practical30-production-check.txt`; build and flash traces remain local
bench artifacts. Host startup, recovery and deadline logic
tests passed (`19-host-tests.txt`). This remains bench verification, not
driving or eye-recognition validation.
The previous 85 FPS production capture was interrupted when the user chose
30 FPS; its last report is at about 96 seconds and the log is named
`19-production-interrupted-96s.log`, despite its original 125-second request.

## Earlier maximum-throughput experiments

The selected candidate preserves the original sensor window (0,0 to
2079,1547), outputs 320x240 RGB565 and uses 4x subsampling. It does not
crop the field of view, but sampling changes spatial detail and aliasing.

| Configuration | Measured FPS | Result |
| --- | ---: | --- |
| Original full field | 14.807 | baseline |
| PLL SYSCLK 53.333 MHz, PCLK 17.778 MHz | 29.615 | valid |
| Full-field 4x sampling, HTS 1600, VTS 392 | 85.033 | experimental, not selected for application |
| Full-field 4x sampling, HTS 2300 or 1150 | — | rejected: no frames |
| Cropped 1280 region | 70.329 | excluded: user requires full field |

Evidence: `17-fps-sweep.log` and `18-full-fov-sweep.log` include firmware
SHA, register readback and independent capture-timestamp FPS. The experimental
85 FPS 20-second sample has 1,701 accepted frames, zero bad frames, zero repeated
or backward timestamps, and 1,700 changed sampled hashes. Counts can be
recalculated from the timestamped frame samples in the logs.

Synthetic sensor color bars were inspected in `18-patterns/`; no scene or
face image was saved. This checks transport/scaling appearance, not eye
recognition accuracy. The fastest passing candidate is not a proof of the
absolute hardware maximum. Incomplete USB logs numbered 16 are excluded.

The regular application applies the selected preset on startup and recovery.
CPU 240 MHz, octal PSRAM 80 MHz, direct PSRAM DMA, two buffers. The USB-native build uses COM4 and primary
USB Serial/JTAG console; repository defaults retain UART console for CH343.

Sources consulted:

- [OmniVision OV3660 datasheet, hosted by Seeed](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/res/OV3660_datasheet.pdf): PLL limits, timing and subsampling registers. Scaling alone does not increase sensor frame rate.
- [Espressif OV3660 driver](https://github.com/espressif/esp32-camera/blob/master/sensors/ov3660.c): sensor timing API and default presets; implementation uses the pinned local 2.1.9 source.

The sweep firmware and local checker remain bench artifacts; the regular
application uses only the full-field ~30 FPS profile.
