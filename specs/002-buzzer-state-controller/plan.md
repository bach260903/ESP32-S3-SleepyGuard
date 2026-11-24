# Implementation Plan: Còi, LED và máy trạng thái cảnh báo

**Branch**: main (feature 002-buzzer-state-controller) | **Date**: 2026-11-24 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/002-buzzer-state-controller/spec.md`

## Summary

Giữ camera feature 001 đang chạy khoảng 29,6 FPS; thêm logic cảnh báo thuần nhận sự kiện giả lập và sự kiện lỗi camera, một bộ phát mẫu còi/LED không chặn, cùng lệnh thử qua Serial. Không dùng dữ liệu ảnh. Việc đầu tiên khi triển khai là phép thử chân và xác định mạch còi có đệm, ghi log từng chân và quan sát của người dùng trước khi chốt bảng chân.

## Technical Context

**Language/Version**: C với ESP-IDF v5.5.2; host GCC cho logic thuần.
**Primary Dependencies**: ESP-IDF v5.5.2, espressif/esp32-camera 2.1.9 đã ghim; FreeRTOS, GPIO; LEDC chỉ khi mạch còi thụ động đã có tầng đệm.
**Storage**: RAM; log văn bản không chứa ảnh; không dùng microSD hoặc mạng.
**Testing**: assertion PC với đồng hồ giả; build/nạp ESP32-S3, capture Serial có timestamp và ghi quan sát còi/LED trên bàn.
**Target Platform**: ESP32-S3 CAM N16R8, OV3660, USB-UART CH343 hoặc USB Serial/JTAG theo cổng hiện có.
**Project Type**: firmware cảnh báo thử nghiệm trên bàn.
**Performance Goals**: phản hồi tắt/báo trong 100 ms; nhịp lỗi 5 s ±200 ms; camera 125 s không sinh lỗi/báo động ở đầu vào MỞ.
**Constraints**: không cấp dòng trực tiếp từ GPIO cho còi/loa không đệm; chân/cực tính phải đo; không làm chậm worker camera; không lưu/truyền ảnh; không dùng ngoài đường thật.
**Scale/Scope**: một camera, một còi, một LED, một nguồn sự kiện giả lập; chưa tích hợp suy luận mắt thật.

## Constitution Check

| Nguyên tắc | Gate trước nghiên cứu | Gate sau thiết kế |
| --- | --- | --- |
| I Đường cảnh báo độc lập | Logic và bộ phát không chờ ghi log/SD; camera worker không chờ Serial | PASS: contract lệnh chỉ dùng ở bench, bộ phát tick không chặn |
| II Lỗi camera tường minh | Mẫu lỗi riêng bắt đầu khi chốt lỗi, lặp 5 s | PASS: trạng thái lỗi có mã/nguyên nhân và log định kỳ |
| III Test PC và board | Test thuần trước mã, log và quan sát board ở từng checkpoint | PASS: T001 và từng story có bằng chứng board |
| IV Hai mức và một nơi cấu hình | 0,75 s/1,5 s cùng cấu hình; tắt qua MỞ/ACK | PASS: data-model và hợp đồng chuyển trạng thái |
| V Không lưu ảnh | Chỉ sự kiện và số đo thời gian | PASS |
| VI Bàn thử/USB | quickstart ghi giới hạn, không thử khi lái | PASS |
| VII Feature nhỏ | Tách test chân, logic, tích hợp, xác minh | PASS |
| D-013 mạch còi | Chỉ bật chân còi sau khi xác nhận mạch đệm; nếu không có, dừng bước còi | PASS: T001 là gate phần cứng |

## Thứ tự thực hiện và gate đầu tiên

1. **T001 thử chân trước mọi logic còi**: sao lưu cấu hình bản thường; dùng firmware thử hữu hạn để bật/tắt từng chân ứng viên riêng lẻ (LED trước, còi sau), log `PIN_PROBE gpio=... level=... elapsed_ms=...` và thu file Serial. Dùng mắt/tai quan sát LED/còi tại từng nhịp, ghi có/không và cực tính vào `reports/002-alert/01-pin-probe-check.md`. Không thử chân camera, flash, PSRAM hay USB; liệt kê ứng viên và loại trừ trong báo cáo. Chỉ thử chân còi sau khi kiểm tra mạch đệm, mức điện áp, cực tính và giới hạn dòng của mạch. Nếu là loa/còi hai dây không đệm, dừng phần còi, yêu cầu module active 3,3 V có đệm hoặc tầng transistor được chứng minh trước khi cấp tín hiệu. Với còi thụ động đã đệm, dùng phép thử âm tần giới hạn thời lượng; với active module, thử bật/tắt. Mỗi ứng viên là một lần thử có mã và kết quả riêng, không quét đồng thời.
2. Chốt bảng chân và loại driver trong `main/board_pins.h` và `reports/002-alert/01-pin-probe-check.md` **chỉ từ log cộng quan sát**; ảnh nhãn và giá trị GPIO2/GPIO3 của thử nghiệm cũ là ứng viên, không phải kết luận.
3. Cài logic thuần và test PC, sau đó driver còi/LED tick không chặn.
4. Tích hợp sự kiện lỗi camera feature 001; lệnh giả lập có thể kiểm tra logic nhưng không tự tạo khung hình.
5. Kiểm chứng từng story bằng log và quan sát, cuối cùng nạp bản thường với chế độ thử chân tắt và chạy 125 giây.

## Project Structure

### Documentation (this feature)

```text
specs/002-buzzer-state-controller/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/serial.md
└── tasks.md
reports/002-alert/
├── 01-pin-probe.log
├── 01-pin-probe-check.md
└── ... checkpoint logs and checks
```

### Source Code (repository root)

```text
main/
├── board_pins.h
├── bringup_config.h
├── bringup_main.c
└── Kconfig.projbuild
components/
├── alert_logic/include/alert_logic.h
├── alert_logic/alert_logic.c
├── board_io/include/alert_output.h
├── board_io/alert_output.c
└── board_io/bringup_camera.c
tests/host/
├── test_alert_logic.c
└── run_tests.ps1
tools/
└── capture_serial.py
```

**Structure Decision**: logic thời gian và trạng thái nằm trong component thuần, không include ESP-IDF. Driver phần cứng nằm trong board_io. main nối sự kiện camera và lệnh thử với logic; worker camera không điều khiển còi hay chờ Serial. Chế độ pin probe chỉ chạy khi bật cờ bench và không nằm trong đường ứng dụng thường.

## Design notes

- Đầu vào Serial là nguồn thử có cú pháp cố định; sự kiện camera thật có ưu tiên cao hơn lệnh mắt giả lập khi camera không khỏe.
- Mẫu NGHI NGỜ, BÁO ĐỘNG và LỖI CAMERA là nhịp âm/ánh sáng. Active buzzer chỉ hỗ trợ bật/tắt, vì vậy “bíp nhẹ/còi lớn” được phân biệt bằng nhịp và thời lượng, không hứa tăng âm lượng phần cứng. Còi thụ động cần PWM âm tần qua tầng đệm; nhịp cấp cao không đổi.
- Khi FAULT camera hết lượt, bộ phát vẫn tick và lặp mẫu lỗi; queue camera vẫn được rút như feature 001.
- GPIO3 là chân strapping theo quyết định cũ; nếu đo thực tế chọn GPIO3 cho LED, phải kiểm tra điện trở/mạch ngoài không làm sai boot khi cấp nguồn và nhấn reset. Không dùng nhãn ảnh làm bằng chứng.
- Sau khi thiết kế, không còn NEEDS CLARIFICATION về kiến trúc. Chân, cực tính và loại mạch là kết quả phải đo ở gate T001, không điền trước.
