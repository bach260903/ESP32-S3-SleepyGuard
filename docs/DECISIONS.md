# Nhật ký quyết định

Mỗi quyết định có mã `D-xxx`, trạng thái và ngày. Quyết định mới được thêm ở cuối;
quyết định bị thay thế thì đổi trạng thái thành `Superseded` và trỏ tới mã mới.

Trạng thái: **Accepted** (đã chấp nhận) · **Open** (chưa chốt, cần kiểm chứng).

---

## D-001 – Phần cứng nền tảng
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Dùng board ESP32-S3 module N16R8 (16 MB flash, 8 MB PSRAM octal)
  và camera OV3660.
- **Board thực tế (xác nhận qua ảnh 2026-11-19):** board ESP32-S3 CAM có chữ in
  "ESP32-S3 WROOM", module ESP32-S3-N16R8, camera OV3660 cắm bằng đầu nối FFC,
  có khe thẻ nhớ (SD Card), hai cổng USB-C (USB-UART và USB-OTG). Nạp code và
  xem log qua cổng USB-UART. Bảng chân chính thức từ người bán vẫn chưa xác
  nhận; bảng chân vận hành bên dưới dựa trên phép thử với firmware thật.
- **Link sản phẩm do người dùng cung cấp:** https://vn.shp.ee/95xYYkgm
  → https://shopee.vn/product/1240998350/54851568148.
  Ngày 2026-11-19: đã xác định URL đích; Shopee chặn truy cập tự động nên
  chưa đọc được nội dung người bán và chưa xác nhận bảng chân.
- **Bảng chân thực nghiệm từ firmware cũ:** cấu hình trong bản sao lưu
  `firmware/main/camera_utils.cpp` và `firmware/sdkconfig` đã chạy trên board
  ESP32-S3 N16R8 + OV3660; `firmware/reports/board_selftest.log` ghi PID
  `0x3660`, khung 240×240 và `SELFTEST cam: PASS`. Đây là bằng chứng vận hành
  của cấu hình, không phải sơ đồ mạch chính thức. Ngày 2026-11-19 đã nạp lại
  firmware cũ trên chính board và đọc COM4: `healthy=1 sensor=OV3660`, FPS 6,2.
  Chi tiết GPIO và log mới ghi trong plan feature 001. Phép thử này dùng
  firmware cũ 240×240, chưa xác nhận firmware mới 320×240.
- **Đo trên board với firmware mới (2026-11-19):** `esp_psram_is_initialized()`
  trả `1`, `esp_psram_get_size()` trả thô **8 388 608 byte**; heap PSRAM còn
  trống **8 386 196 byte**. Flash vật lý báo **16 777 216 byte** (16 MB).
  Bằng chứng trong `reports/001-camera/01-psram-flash.log` và
  `reports/001-camera/02-phase1-check.md`. Probe mới nhận OV3660 PID `0x3660`
  và ba khung RGB565 320×240 hợp lệ trong `02-camera-probe.log`.
- **Luồng camera và phục hồi trên board (2026-11-21):** bản firmware feature 001
  chạy OV3660 320×240 qua USB-UART/CH343; log US1 125 giây và các ca US3 có
  khung hợp lệ thật sau deinit → init. Xem `reports/001-camera/04b-us1-tuned-125s.log`
  và `reports/001-camera/06-us3-check.md`.
- **Hệ quả:** Phải bật PSRAM chế độ octal và cấu hình flash 16 MB. Các chân GPIO
  mà PSRAM octal chiếm (khoảng GPIO35–37) không dùng được cho việc khác. Khi chọn
  chân cho còi, tránh các chân strapping (GPIO0, 3, 45, 46).

## D-002 – Mục tiêu chức năng
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Nếu mắt tài xế nhắm liên tục quá X giây thì phát còi cảnh báo.
  Xử lý hoàn toàn trên thiết bị.

## D-003 – Framework
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** ESP-IDF, cài qua tiện ích Espressif IDF cho VS Code.
- **Phiên bản ghim cho feature 001 (2026-11-19):** ESP-IDF **v5.5.2**,
  target `esp32s3`; component `espressif/esp32-camera` **==2.1.9**.
  `dependencies.lock` đã được lưu. **ESP-IDF v5.5.2 và esp32-camera 2.1.9
  đã xác nhận bằng build và chạy thật (2026-11-19)**; xem
  `reports/001-camera/01-build.txt` và `02-camera-probe.log`. Bản thường sau
  US3 build sạch với fault injection tắt và không có chuỗi scenario trong
  `.elf`/`.map`; xem `reports/001-camera/07-production-build.txt` và
  `07-production-binary-check.txt`.
  Nguồn: https://github.com/espressif/esp-idf/releases/tag/v5.5.2 và
  https://components.espressif.com/components/espressif/esp32-camera/versions/2.1.9/dependencies.
- **Lý do:** Hỗ trợ camera, PSRAM và các thư viện AI của Espressif tốt nhất; build
  bằng dòng lệnh nên phù hợp với quy trình dùng agent.
- **Hệ quả:** Phải ghim (pin) phiên bản ESP-IDF cụ thể khi bring-up và ghi lại ở đây.
  Người làm chưa có kinh nghiệm vi điều khiển, nên mỗi feature phải nhỏ và có
  hướng dẫn chạy từng bước.

## D-004 – Cách phát hiện nhắm mắt
- **Trạng thái:** Accepted (hướng đi) · Open (mô hình cụ thể) · 2026-11-19
- **Quyết định:** Pipeline gồm phát hiện mặt → cắt vùng mắt → phân loại mắt mở/nhắm →
  bộ đếm thời gian nhắm.
- **Open:** Chưa chọn mô hình. Ứng viên: các thư viện ESP-DL / ESP-WHO của Espressif
  hoặc mô hình tự huấn luyện. Cần một spike nghiên cứu với tiêu chí: FPS tối thiểu và
  độ chính xác trên chính board này.
- **Hiệu năng camera ở 320×240 RGB565 (2026-11-20):** cấu hình ban đầu (một
  framebuffer, XCLK 10 MHz, PSRAM 40 MHz) đo 2,36–2,78 FPS. Sau khi tăng lần
  lượt XCLK, số framebuffer và tốc độ PSRAM, cấu hình XCLK 24 MHz, hai
  framebuffer, PSRAM octal 80 MHz đo 14,17–14,81 FPS trong 125 giây,
  `invalid=0`. `grab_mode` vẫn là `CAMERA_GRAB_WHEN_EMPTY`. Log cũ tại
  `reports/001-camera/04-us1-stream-125s.log`; chuỗi thử tại
  `09-fps-optimization.md` và log xác nhận 125 giây tại
  `04b-us1-tuned-125s.log`. Thử thêm buffer thứ ba không tăng FPS đo được;
  giữ hai buffer. Xem `04b-us1-tuned-check.md`.
  FPS này chưa gồm thời gian chạy mô hình.

## D-005 – Ngưỡng X
- **Trạng thái:** Accepted (giá trị khởi đầu) · 2026-11-19
- **Quyết định:** Mặc định X = 1,5 giây. Mọi ngưỡng đặt ở một nơi cấu hình duy nhất,
  không hard-code rải rác. Giá trị cuối cùng chốt bằng thực nghiệm.

## D-006 – Kiểu báo động
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Hai mức: bíp nhẹ khi nghi ngờ, còi lớn khi vượt X. Còi tắt khi
  mắt mở lại hoặc khi bấm nút.

## D-007 – Xử lý lỗi camera
- **Trạng thái:** Accepted (nguyên tắc) · Open (thông số) · 2026-11-19
- **Quyết định:** Lỗi camera (khởi tạo thất bại, mất khung hình, hình bất thường)
  không được xử lý trong im lặng. Thiết bị báo cho tài xế bằng mẫu bíp riêng, thử
  khởi tạo lại có giới hạn, và nếu không phục hồi thì tiếp tục báo định kỳ.
- **Phạm vi feature 001 (2026-11-19):** firmware chẩn đoán camera trên bàn thử
  chỉ báo lỗi có mã và định kỳ qua Serial; không phát còi. Mẫu bíp lỗi vẫn là
  yêu cầu của thiết bị cảnh báo hoàn chỉnh và được chốt trong feature còi.
- **Bằng chứng feature 001 (2026-11-21):** mất khung được chốt sau 3 khung lỗi
  liên tiếp hoặc 5 giây không có khung hợp lệ; ngân sách phục hồi là 3 lượt,
  cấp lại sau 60 giây ổn định. Worker cũ không dừng trong 15 giây sau yêu cầu
  dừng thì báo `WORKER_STOP_TIMEOUT`; supervisor vẫn nhận sự kiện muộn nhưng
  không khởi tạo lại. Xem `reports/001-camera/06-us3-check.md`.
- **Khoảng không giám sát được:** ca phục hồi nhanh quan sát khoảng 5 giây để
  phát hiện mất khung cộng khoảng 0,8 giây phục hồi, xấp xỉ **6 giây** không
  có khung hợp lệ mới. Ca worker chậm có thể dài hơn. Feature còi phải coi
  toàn bộ khoảng này là lỗi camera cần báo cho tài xế bằng mẫu bíp riêng;
  không chờ phục hồi xong rồi mới báo.
- **Open:** Mẫu bíp lỗi camera cụ thể cho thiết bị hoàn chỉnh sẽ chốt trong
  feature còi.

## D-008 – Kiểm thử
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Logic thuần (máy trạng thái, bộ đếm) tách riêng để kiểm thử trên PC.
  Mọi thứ chạm tới camera, còi hoặc thời gian phản hồi phải có bằng chứng chạy trên
  board thật (ESP32-S3 N16R8 + OV3660).

## D-009 – Quyền riêng tư
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Xử lý tại thiết bị. Không lưu ảnh hay video khuôn mặt; không gửi
  dữ liệu ra ngoài. Muốn đổi phải có quyết định mới ở đây.

## D-010 – Điều kiện ánh sáng
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Phiên bản 1 chỉ hỗ trợ ban ngày. OV3660 thường không có tính năng
  hồng ngoại; ban đêm cần camera NoIR và đèn IR, để dành cho phiên bản sau.

## D-011 – Nguồn điện
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Giai đoạn đầu cấp nguồn qua USB. Mạch hạ áp từ 12 V trên xe,
  lọc nhiễu và vấn đề nhiệt để sau.

## D-012 – Giới hạn trách nhiệm
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** README ghi rõ đây là hệ thống hỗ trợ, không thay thế nghỉ ngơi
  khi lái xe, và chưa được kiểm chứng để dùng trên đường thật.

## D-013 – Loại còi
- **Trạng thái:** Accepted (cần xác nhận khi có phần cứng) · 2026-11-19
- **Quyết định:** Dùng còi chủ động (active buzzer) dạng module có sẵn mạch đệm,
  điều khiển bằng bật/tắt GPIO theo mẫu thời gian. Không kéo còi trực tiếp từ GPIO
  nếu không có mạch đệm.
- **Lý do:** Đơn giản nhất cho người mới; mẫu bíp tạo bằng thời gian bật/tắt là đủ cho D-006 và D-007.

## D-014 – Quy trình phát triển
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Dùng Spec Kit. Mỗi feature là một lát cắt nhỏ, kiểm tra được trên
  board. Thứ tự: constitution → specify → clarify → plan → tasks → implement.
  Quyết định kiến trúc mới phải ghi vào file này.

## D-015 – Đường cảnh báo độc lập với tính năng phụ
- **Trạng thái:** Accepted · 2026-11-19
- **Quyết định:** Đường cảnh báo (camera → phát hiện → còi) không được phụ thuộc vào
  tính năng phụ như ghi log hay microSD. Lỗi hoặc vắng mặt của tính năng phụ không
  làm đổi hành vi cảnh báo, và việc ghi log không được chặn hay làm chậm vòng lặp
  phát hiện.

---

## Câu hỏi còn mở

- Board đã xác nhận có khe microSD; bảng chân camera và các chân còn trống
  vẫn cần tài liệu chính thức của người bán (D-001).
- Mô hình nhận diện mắt nào đạt FPS và độ chính xác chấp nhận được? (D-004)
- Giá trị X cuối cùng, và tỷ lệ báo nhầm chấp nhận được mỗi giờ?
- Mẫu bíp khi lỗi camera cho thiết bị hoàn chỉnh? (D-007)
