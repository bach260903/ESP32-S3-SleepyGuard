# OV3660 QVGA RGB565 FPS investigation — 2026-11-20

> Correction (2026-11-22): XCLK values below are requested frequencies.
> The pinned S3 driver uses an integer divider from 160 MHz. The 24 MHz
> request selects divider 6, implying nominal 26.667 MHz XCLK, not 24 MHz.
> Sensor "Calculated PCLK" messages use the request and are not pin
> measurements. See `14-fps-followup.md`. Raw FPS observations are unchanged.

Board: ESP32-S3 N16R8 with OV3660 on CH343 COM5. Firmware uses ESP-IDF
5.5.2 and `espressif/esp32-camera` 2.1.9. Every variant kept 320×240 RGB565,
returned each frame immediately, and reported valid frames over actual elapsed
time in five-second windows. Each row changes one setting from the row above.

| Configuration | Steady FPS | Invalid frames | Capture |
| --- | ---: | ---: | --- |
| XCLK 10 MHz, 1 FB, PSRAM 40 MHz | 2.78 | 0 | `04-us1-stream-125s.log` |
| XCLK 16 MHz, 1 FB, PSRAM 40 MHz | 4.36–4.53 | 0 | `05-xclk16-stream-32s.log` |
| XCLK 20 MHz, 1 FB, PSRAM 40 MHz | 5.55 | 0 | `06-xclk20-stream-32s.log` |
| XCLK 20 MHz, 2 FB, PSRAM 40 MHz | 7.13–7.73 | 0 | `07-xclk20-fb2-stream-42s.log` |
| XCLK 20 MHz, 2 FB, PSRAM 80 MHz | 11.10–11.11 | 0 | `08-psram80-stream-42s.log` |
| XCLK 24 MHz, 2 FB, PSRAM 80 MHz | 14.81 | 0 | `10-xclk24-stream-42s.log`, `11-xclk24-stream-125s.log` |
| XCLK 24 MHz, 3 FB, PSRAM 80 MHz | 14.80–14.81 | 0 | `04c-fb3-42s.log` |

The OV3660 driver calculates its QVGA RGB565 PCLK from XCLK: the captured
driver log shows 5, 8, 10, and 12 MHz PCLK at 10, 16, 20, and 24 MHz XCLK. The
driver's single-buffer mode starts capture on VSYNC and stops after the frame;
two buffers allow continuous capture. Raising octal PSRAM from 40 to 80 MHz
made the largest additional difference, consistent with RGB565 writes to
external RAM being costly. See the pinned driver's `sensors/ov3660.c` and
`README.md` for these mechanisms.

Selected configuration: XCLK 24 MHz, two RGB565 framebuffers, octal PSRAM
80 MHz. The 80 MHz setting is in `sdkconfig.defaults` so clean builds use it.
`grab_mode` remained `CAMERA_GRAB_WHEN_EMPTY` throughout. A fresh 125-second
confirmation in `04b-us1-tuned-125s.log` again had 24 reports, 1,796 valid
frames, zero invalid frames, and 14.81 steady FPS. A third framebuffer added
153,600 bytes of PSRAM use but did not improve steady FPS, so the board was
returned to two buffers. See `04b-us1-tuned-check.md`.
The 20 MHz, 125-second run in `09-optimized-stream-125s.log` yielded 24
five-second reports, 1,341 valid frames, zero invalid frames, and no camera
warnings or faults. After the first window, every report was 11.10–11.11 FPS.
Following the OV2640 tuning example's suggestion to vary XCLK, a 24 MHz test
on this OV3660 changed the driver's calculated PCLK log from 10 to 12 MHz
(not a physical PCLK measurement). Its 125-second run yielded 24
reports, 1,796 valid frames, zero invalid frames, and no camera warnings or
faults. After the first window, every report was 14.81 FPS, about 33% faster
than 20 MHz and 5.3 times the original 2.78 FPS.

The linked example uses JPEG and Wi-Fi, whereas this firmware uses RGB565
without networking. `jpeg_quality` is only applied by `esp_camera_init()`
for `PIXFORMAT_JPEG`, so adjusting it cannot speed up this RGB565 capture.
Changing gain ceiling would change exposure behavior and needs an image-quality
evaluation; no gain setting was changed in this experiment. A JPEG capture
benchmark would measure a different image format and would need decoding before
an RGB-based eye detector could consume it.

This measurement covers capture and frame validity checks, not later face or
eye processing. Frame validity currently checks dimensions and byte count;
image content was not visually inspected. The driver README cautions that
multiple buffers with raw RGB put extra load on memory, so monitor invalid
frames and camera warnings when adding image processing.

The ESP-IDF host smoke and logic compilation checks passed. The build, flash,
and serial logs for each variant are retained alongside this report.
