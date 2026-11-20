# Implementation Plan: Bring-up camera OV3660

**Branch**: main (feature 001-bringup-ov3660-camera)
**Date**: 2026-11-19
**Spec**: [spec.md](spec.md)
**Status**: Setup, foundation, US1 và US2 (T001–T018) hoàn tất; US3 chưa làm.

## Summary

Dùng ESP-IDF v5.5.2, target esp32s3 và component espressif/esp32-camera ==2.1.9.
Kiểm tra PSRAM trước camera; lấy khung 320 × 240 và báo số khung hợp lệ/lỗi, FPS
qua USB-UART. Phục hồi có ngân sách hữu hạn và trạng thái lỗi vẫn báo định kỳ.
Firmware probe và US1 đã build, chạy trên board; bằng chứng tại
`reports/001-camera/02-phase1-check.md` và `04-us1-check.md`. Logic phục hồi
còn ở phase sau.

**Thứ tự triển khai:** cài ESP-IDF v5.5.2, build/nạp khung tối thiểu có
esp32-camera 2.1.9 và in PSRAM/flash trước; sau đó kiểm chứng lại bảng chân
với camera 320×240 trên board hiện tại. Người dùng đã chọn phương án xác minh
thực tế thay cho đợi tài liệu người bán. Không coi bảng chân này là sơ đồ mạch
chính thức.

## Technical Context

**Language/Version**: C theo toolchain đi kèm ESP-IDF v5.5.2.
**Primary Dependencies**: ESP-IDF v5.5.2; espressif/esp32-camera ==2.1.9; lưu
dependencies.lock. Cặp phiên bản đã build và chạy trên board trong Phase 1.
**Storage**: RAM nội và PSRAM; không lưu ảnh/video, không dùng SD hay mạng.
**Testing**: kiểm thử logic thời gian/ngân sách trên PC; kiểm chứng PSRAM, camera,
thời hạn và Serial trên board thật. Log chỉ chứa metadata, không có ảnh.
**Target Platform**: ESP32-S3 CAM N16R8, flash 16 MB, PSRAM octal 8 MB, OV3660 FFC;
cấp nguồn, nạp và xem log qua USB-UART.
**Project Type**: firmware chẩn đoán trên bàn thử.
**Performance Goals**: đo FPS thực tế, không cam kết FPS nhận diện; 24 báo cáo
5 giây trong cửa sổ quan sát 125 giây sau khi camera sẵn sàng;
không có reset tự động do logic lỗi.
**Constraints**: PSRAM test trước camera; mọi ngưỡng ở một cấu hình chung; thời hạn
mất khung và chờ khung phục hồi là hai tham số độc lập; GPIO không được suy đoán.
**Scale/Scope**: một board, một camera, một luồng khung; không nhận diện hoặc còi.
**Camera pin mapping**: đã có bằng chứng chạy trên board từ bản sao lưu và
kiểm chứng lại ngày 2026-11-19 bằng log mới trên COM4.

### Bảng chân đã từng chạy trên board và phép thử lại

- Nguồn cấu hình: bản sao lưu ngoài repo
  `D:/ESP32-canh_bao_nguoi_ngu_gat-backup-20261119/firmware/main/camera_utils.cpp`
  dòng 20–36 và `firmware/sdkconfig` dòng 533–548.
- Bằng chứng lần chạy trước: `firmware/reports/board_selftest.log` trong cùng
  bản sao lưu, các dòng 181–199 ghi SCCB SDA 4/SCL 5, PID 0x3660, khung
  240×240 trong PSRAM và camera selftest PASS.

| Tín hiệu | GPIO đã chạy |
|---|---:|
| PWDN | -1 (không điều khiển bằng GPIO) |
| RESET | -1 (không điều khiển bằng GPIO) |
| XCLK | 15 |
| SCCB SDA / SCL | 4 / 5 |
| D0 / D1 / D2 / D3 | 11 / 9 / 8 / 10 |
| D4 / D5 / D6 / D7 | 12 / 18 / 17 / 16 |
| VSYNC / HREF / PCLK | 6 / 7 / 13 |

- Cấu hình cũ dùng XCLK 10 MHz, RGB565, 240×240 và 4 framebuffers.
  Cấu hình bring-up ban đầu chọn 320×240 và một framebuffer; US1 đo
  2,36–2,78 FPS. Sau khi thử từng tham số, cấu hình hiện chọn XCLK 24 MHz,
  hai framebuffer, PSRAM octal 80 MHz và `CAMERA_GRAB_WHEN_EMPTY`. Phép đo
  125 giây ban đầu ghi 14,17–14,81 FPS, `invalid=0`; xem
  `reports/001-camera/09-fps-optimization.md` và log xác nhận
  `04b-us1-tuned-125s.log`. Thử ba buffer không tăng FPS; giữ hai buffer.
- Ngày 2026-11-19, firmware cũ đã nạp thành công lên ESP32-S3 MAC
  `d8:85:ac:ca:1b:e4` qua CH343 COM5 và xác minh hash. Sau khi cắm lại,
  USB Serial/JTAG COM4 đã cho log mới: `camera healthy=1 sensor=OV3660
  attempts=1` trong [board-camera-status.log](board-camera-status.log),
  cùng `FPS=6.2` trong [board-pin-verification.log](board-pin-verification.log).
  Cấu hình pin hoạt động thực tế với firmware cũ 240×240; chưa chứng minh
  mức 320×240 của feature mới hoặc ESP-IDF v5.5.2.
- Sau phép thử, đã xóa toàn bộ flash qua COM4; esptool báo xóa thành công
  trong 7,9 giây. Board không còn chạy firmware cũ.
- Pinout vận hành đã có bằng chứng; phép đo FPS liên tục với cấu hình mới
  vẫn thuộc US1.
- Ngày 2026-11-20, firmware probe ESP-IDF v5.5.2 + esp32-camera 2.1.9 đã nhận
  OV3660 PID 0x3660 và ba khung RGB565 320×240 dài 153 600 byte qua USB-UART.
  Log tại `reports/001-camera/02-camera-probe.log`. FPS liên tục vẫn cần US1.

### Cấu hình đã có cơ sở nghiên cứu

Phiên bản IDF được ghim trong docs/DECISIONS.md, không tự dùng v5.3 đang có sẵn
trên máy. Việc cài toolchain v5.5.2 sẽ thuộc triển khai.

sdkconfig.defaults sẽ là nguồn cấu hình build, tạo sdkconfig với target esp32s3:
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y, CONFIG_SPIRAM=y, CONFIG_SPIRAM_MODE_OCT=y,
CONFIG_SPIRAM_SPEED_80M=y, CONFIG_SPIRAM_BOOT_INIT=y,
CONFIG_SPIRAM_IGNORE_NOTFOUND=y,
CONFIG_SPIRAM_USE_CAPS_ALLOC=y. Tắt CONFIG_SPIRAM_MEMTEST và các tùy chọn đưa
BSS/NOINIT/instructions/rodata ra PSRAM. Dùng console UART mặc định qua USB-UART.
Kiểm tra lại sdkconfig sau khi sinh; không chỉ tin tệp defaults.

Một file main/bringup_config.h dự kiến chứa tất cả tham số ứng dụng:
- camera 320 × 240, RGB565; hai framebuffer trong PSRAM (mỗi cái 153 600 byte),
  XCLK 24 MHz, `CAMERA_GRAB_WHEN_EMPTY`;
- số lần init ban đầu 3, ngân sách recovery 3;
- số lỗi liên tiếp 3, mất khung 5000 ms;
- chờ khung recovery 5000 ms (tham số riêng), ổn định 60000 ms;
- báo thống kê 5000 ms, báo lỗi cuối 5000 ms (hai tham số riêng);
- dung lượng kỳ vọng, kích thước vùng PSRAM test suy ra từ cấu hình khung.
Không sao chép các ngưỡng này vào nhiều module. sdkconfig chứa cấu hình build
phần cứng, không phải nơi thứ hai định nghĩa ngưỡng ứng dụng. GPIO đã xác minh
sẽ thuộc board_pins.h riêng, không chứa ngưỡng hành vi.

### Đối chiếu dung lượng PSRAM (FR-001)

- `PSRAM_UNAVAILABLE`: `esp_psram_is_initialized()` trả false.
- `PSRAM_CONFIG_MISMATCH`: đã khởi tạo nhưng `esp_psram_get_size()` khác
  **8 MiB = 8 388 608 byte**. So sánh bằng theo hạng dung lượng N16R8,
  không dùng dung sai phần trăm. Header ESP-IDF v5.5.2 mô tả API này là
  dung lượng khả dụng của chip; cần kiểm tra giá trị thực tế lúc build với
  cấu hình ECC/XIP đã tắt trước khi coi phép so sánh này là hợp lệ.
- Firmware phải in nguyên giá trị `esp_psram_get_size()` theo byte trong
  `BOOT_INFO` ở lần chạy đầu, cùng giá trị kỳ vọng 8 388 608 byte. Lưu log
  thực tế trên board trước khi chốt quy tắc so sánh. Nếu hai giá trị khác
  nhau, ghi nhận và cập nhật plan theo dữ liệu board; không nới dung sai để
  che khác biệt.
- Dung lượng heap PSRAM còn trống chỉ in để tham khảo, không dùng phán đoán.
- Nguồn: https://raw.githubusercontent.com/espressif/esp-idf/v5.5.2/components/esp_psram/include/esp_psram.h

### Việc cần đưa vào tasks

1. Cài ESP-IDF v5.5.2, thêm esp32-camera 2.1.9, build khung tối thiểu và
   nạp thử. Lưu dependency lock, kiểm tra sdkconfig sinh ra.
2. Xác nhận bảng chân vận hành với firmware mới ở 320×240 và console UART qua
   cổng USB-UART/CH343; không giả định số COM cố định.
3. Logic trong `components/bringup_logic/` nhận thời điểm làm tham số.
   Test PC dùng đồng hồ giả cho 4 999/5 000 ms và 59 999/60 000 ms;
   board chạy một hoặc hai ca 60 giây thật.
4. Giả lập lỗi dưới cờ `CONFIG_BRINGUP_FAULT_INJECTION`, mặc định tắt;
   bản build thường không biên dịch mã giả lập.
5. Supervisor chỉ phát tín hiệu deadline và yêu cầu dừng; camera worker sở
   hữu driver, trả frame rồi tự deinit sau khi thoát `fb_get`, sau đó phát
   `BRINGUP_CAMERA_STOPPED`. Đường init lỗi phát `INIT_FAILED` sau cleanup;
   đây cũng là sự kiện kết thúc worker. Supervisor tiếp tục rút queue, bỏ
   FRAME/READY cũ và báo `RECOVERY_WAIT` mỗi 5 giây trong lúc chờ; chỉ init
   lượt kế tiếp sau sự kiện kết thúc. Chờ quá 15 giây thì FAULT
   `WORKER_STOP_TIMEOUT`, không deinit hoặc reboot. Đo thời gian `fb_get` thật
   trên board, tách khỏi ca trì hoãn
   worker giả lập 6 giây. `fb_get` NULL có nghỉ 20 ms và được log khác khung
   sai kích thước. Tạo task thử riêng deadline khi worker còn đang chặn.

Chi tiết nghiên cứu trong [research.md](research.md).

## Constitution Check

| Nguyên tắc | Trước Phase 0 | Sau Phase 1 |
|---|---|---|
| I: cảnh báo độc lập tính năng phụ | Phạm vi bring-up, không SD/mạng | PASS: không có tính năng phụ trong thiết kế |
| II: lỗi tường minh, phục hồi hữu hạn | Serial ngay khi lỗi và định kỳ | PASS: D-007 và constitution v2.0.0 quy định Serial cho firmware chẩn đoán feature 001; mẫu bíp thuộc thiết bị hoàn chỉnh |
| III: kiểm thử board thật | Cần log trên board | PASS thiết kế: quickstart yêu cầu log mới 320×240 |
| IV: cấu hình tập trung | Một file cho tham số ứng dụng | PASS: data-model định nghĩa ngưỡng riêng |
| V: riêng tư | Chỉ metadata qua Serial | PASS: serial contract không có ảnh |
| VI: phạm vi/giới hạn | Bàn thử, ban ngày, USB | PASS: quickstart giới hạn bàn thử |
| VII: feature nhỏ, IDF ghim | v5.5.2 và tài liệu | PASS: pin đã ghi DECISIONS, kiểm chứng gắn với feature 001 |

**Gate kết thúc nghiên cứu: PASS.** Nguồn GPIO thực nghiệm có log mới trên
board hiện tại. Còn gate kiểm thử firmware mới ở giai đoạn triển khai.

## Project Structure

### Documentation (this feature)

- plan.md — bản kế hoạch hiện tại; T001 là bước build khung tối thiểu.
- research.md — kết quả nghiên cứu và điều kiện còn thiếu.
- spec.md và checklists/requirements.md — đầu vào đã có.
- data-model.md, contracts/serial.md, quickstart.md — thiết kế Phase 1.
- tasks.md — danh sách task đã sinh; bắt đầu bằng kiểm chứng toolchain.

### Source Code (repository root)

Cấu trúc đề xuất, chưa tạo mã nguồn:
- CMakeLists.txt, sdkconfig.defaults, dependencies.lock
- main/bringup_main.c, main/bringup_config.h, main/board_pins.h
- main/idf_component.yml
- components/bringup_logic/ — logic thời gian, ngân sách và trạng thái thuần
- components/board_io/ — PSRAM, camera owner, Serial
- tests/host/ — CMake, smoke test và các ranh giới 3 lỗi, 5 giây, 60 giây
- tools/capture_serial.py — thu log Serial không tương tác bằng pyserial
- main/Kconfig.projbuild và components/board_io/ — kịch bản lỗi có kiểm soát
- reports/001-camera/ — log và kết quả kiểm thử board sau triển khai

**Structure Decision**: một firmware ESP-IDF ở gốc; tách logic thuần để kiểm thử PC.
Không đưa bảng chân vào source trước khi khung ESP-IDF đã build và nạp được.

## Complexity Tracking

| Điểm cần giải thích | Lý do | Phương án không chọn |
|---|---|---|
| Chưa có bíp lỗi trong feature 001 | D-007 và constitution II giới hạn feature này ở chẩn đoán Serial trên bàn thử | Mẫu bíp được chốt ở feature còi cho thiết bị hoàn chỉnh |
| Camera worker và deadline supervisor dự kiến | Driver vẫn dùng timeout `fb_get` 4 giây ở bản ghim; hai buffer hoạt động liên tục nên thời điểm trả frame có thể khác cấu hình ban đầu. Deadline 5 giây cần được quan sát độc lập | Vòng đọc đồng bộ có thể vượt deadline khi lần đọc bắt đầu gần mốc 5 giây |

**Constitution Check sau Phase 1:** thiết kế đáp ứng các nguyên tắc I–VII theo
D-007 và constitution v2.0.0. Firmware chẩn đoán feature 001 báo lỗi qua Serial;
mẫu bíp là yêu cầu của thiết bị hoàn chỉnh. Bằng chứng chạy firmware mới vẫn là
gate triển khai.

