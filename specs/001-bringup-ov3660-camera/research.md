# Research: Bring-up OV3660

Ngày: 2026-11-19. Trạng thái: Phase 0 hoàn tất bằng xác minh thực nghiệm pinout.
Các kết luận dưới đây là nghiên cứu nguồn, không phải kết quả chạy trên board.

## R1 — Board và GPIO (đã xác minh thực nghiệm)

- Decision: dùng bảng chân trong bản sao lưu đã được thử lại trên board hiện tại;
  chi tiết từng GPIO và nguồn tại [plan.md](plan.md).
- Rationale: chữ ESP32-S3 WROOM, N16R8, hai USB-C và FFC không định danh duy nhất
  wiring của board. Không chọn bảng chân từ ví dụ ESP32-S3 EYE/AI Thinker/generic.
- Alternatives considered: suy từ ảnh hoặc board tương tự — bị loại theo yêu cầu.
- Nguồn đã nhận: https://vn.shp.ee/95xYYkgm chuyển tới
  https://shopee.vn/product/1240998350/54851568148. Trình duyệt bị chuyển tới
  trang verify/traffic/error của Shopee ngày 2026-11-19; tìm theo mã sản phẩm
  chưa có kết quả. Chưa xác minh được nội dung người bán.
- Bằng chứng mới: [board-camera-status.log](board-camera-status.log) ghi
  `healthy=1 sensor=OV3660`, [board-pin-verification.log](board-pin-verification.log)
  ghi FPS 6.2 trên COM4. Đây là thử nghiệm firmware cũ 240×240, không phải
  sơ đồ chính thức hoặc chứng cứ chạy phiên bản IDF mới ở 320×240.

## R2 — Framework và dependency

- Decision: ghim ESP-IDF v5.5.2 và espressif/esp32-camera ==2.1.9; target esp32s3.
- Rationale: IDF là bản phát hành chính thức; component yêu cầu IDF >=5.1 và
  có hỗ trợ OV3660/ESP32-S3. Tương thích khai báo chưa thay thế build và test.
- Alternatives considered: IDF v5.3 hiện có trên máy; không tự dùng thay pin;
  phiên bản latest không ghim — không bảo đảm tái lập.
- Sources:
  - https://github.com/espressif/esp-idf/releases/tag/v5.5.2
  - https://components.espressif.com/components/espressif/esp32-camera/versions/2.1.9/readme
  - https://components.espressif.com/components/espressif/esp32-camera/versions/2.1.9/dependencies
- Component source commit: b3f7fe802f41f17fd7a104d517c53f1f62c8414d.
  Khóa dependency khi triển khai; chưa có dependencies.lock ở bước nghiên cứu.

## R3 — PSRAM và chẩn đoán trước camera

- Decision: bật octal PSRAM, cho phép ứng dụng chạy khi PSRAM không được tìm thấy;
  tắt memtest boot mặc định để dùng bài kiểm tra ứng dụng có lỗi xác định.
  Giữ stack, logic điều khiển và logger trong RAM nội.
- Rationale: memtest boot mặc định có thể abort, không đáp ứng yêu cầu Serial định kỳ.
- Alternatives considered: dùng memtest boot mặc định; đưa BSS/rodata ra PSRAM —
  không phù hợp khả năng báo lỗi khi PSRAM thiếu.
- Cấu hình cần kiểm chứng khi triển khai: SPIRAM_IGNORE_NOTFOUND=y,
  SPIRAM_USE_CAPS_ALLOC=y; tắt SPIRAM_MEMTEST, external BSS/NOINIT, fetch instructions,
  rodata ngoài. Không khẳng định xử lý được mọi dạng hỏng điện/phần cứng.
- APIs nguồn: esp_psram_is_initialized(), esp_psram_get_size(); heap capability
  báo free/total riêng. Không so free bytes với 8 MB danh nghĩa.
- Đối chiếu đã chốt tại [plan.md](plan.md): `esp_psram_is_initialized()` false
  là không có PSRAM; `esp_psram_get_size()` khác 8 388 608 byte là sai hạng
  N16R8 khi các tùy chọn ECC/XIP tắt. API trả dung lượng khả dụng của chip;
  heap còn trống được in riêng và không dùng làm điều kiện. Cần kiểm tra giá
  trị runtime và cấu hình đã sinh lúc build v5.5.2 để xác nhận giả định.
- Test: cấp riêng vùng PSRAM tối thiểu 153600 byte nếu dùng RGB565 320 × 240;
  ghi/đọc mẫu theo địa chỉ rồi mẫu đảo bit, ghi size/pattern/result, giải phóng
  trước camera. Không cấp được vùng đủ lớn hoặc đọc sai đều là PSRAM_TEST_FAILED.
- Flash: esp_flash_get_physical_size() để báo dung lượng vật lý; không nhầm với
  esp_flash_get_size() lấy dung lượng theo image header trong IDF v5.5.2.
- Sources:
  - https://raw.githubusercontent.com/espressif/esp-idf/v5.5.2/components/esp_psram/Kconfig.spiram.common
  - https://raw.githubusercontent.com/espressif/esp-idf/v5.5.2/components/esp_psram/esp32s3/Kconfig.spiram
  - https://raw.githubusercontent.com/espressif/esp-idf/v5.5.2/components/esp_psram/include/esp_psram.h
  - https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/peripherals/spi_flash/index.html

## R4 — Khung hình và thời hạn

- Decision: đề xuất QVGA RGB565, một framebuffer PSRAM, không Wi-Fi.
- Rationale: raw frame có kích thước xác định 153600 byte, phù hợp phép đo/test
  bộ nhớ. Đây là điểm khởi đầu để đo; không hứa hẹn FPS hay độ chính xác nhận diện.
- Alternatives considered: JPEG/multiple buffers — thêm biến thiên kích thước,
  không cần cho bring-up này. Espressif cảnh báo RGB/YUV có thể mất dữ liệu khi
  áp lực PSRAM; phải ghi nhận kết quả trên board.
- Source:
  https://components.espressif.com/components/espressif/esp32-camera/versions/2.1.9/readme

- Decision: đề xuất một camera worker sở hữu driver và một supervisor quan sát
  deadline. Không gọi deinit khi fb_get đang chạy hoặc còn giữ framebuffer.
- Rationale: fb_get không có tham số timeout công khai, dùng thời gian chờ 4000 ms.
  Gọi đọc lần hai gần deadline có thể khiến vòng đồng bộ vượt mốc 5 giây.
- Quy tắc cần thiết kế: tại deadline supervisor chốt thất bại đúng một lần,
  từ chối kết quả đến muộn; chờ worker trả lại frame/kết thúc trước cleanup và
  lần init sau. Thời gian cleanup không được biến thành thời gian chờ khung mới.
  Việc chờ worker không được dừng Serial định kỳ. Không cưỡng bức deinit song song.
- Đây là suy luận thiết kế từ source; chưa có thử nghiệm đo jitter hoặc recovery.
- Sources:
  - https://raw.githubusercontent.com/espressif/esp32-camera/b3f7fe802f41f17fd7a104d517c53f1f62c8414d/driver/esp_camera.c
  - https://raw.githubusercontent.com/espressif/esp32-camera/b3f7fe802f41f17fd7a104d517c53f1f62c8414d/driver/cam_hal.c

## R5 — Cấu hình và phạm vi

- Decision: một bringup_config.h cho tất cả ngưỡng ứng dụng; sdkconfig.defaults
  giữ lựa chọn build phần cứng. Pin map riêng chỉ được điền từ nguồn T001.
- Rationale: ngưỡng mất khung 5000 ms và chờ recovery 5000 ms cùng giá trị nhưng
  khác ý nghĩa, phải có hai tên cấu hình khác nhau; lỗi đơn lẻ không reset 60 giây.
- Alternatives considered: hard-code trong nhiều module hoặc gộp hai timeout —
  trái đặc tả đã làm rõ.
- Serial là ngoại lệ phạm vi được người dùng yêu cầu cho bring-up; thiết bị hoàn
  chỉnh vẫn cần bíp lỗi theo D-007. Không mở rộng feature này sang còi.

## Kết luận Phase 0

R1 đã giải quyết ở mức pinout vận hành, dựa trên phép thử trực tiếp với board.
Phase 1 có thể thiết kế; phép thử 320×240 với firmware mới là tiêu chí triển khai.

