# Validation quickstart: OV3660 bring-up

## Điều kiện đầu vào

- Board ESP32-S3 CAM N16R8, OV3660 đã gắn đúng FFC, nguồn USB ổn định.
- GPIO trong [plan.md](plan.md) đã xác minh bằng log firmware cũ; firmware
  mới vẫn cần tự chứng minh 320×240 và ESP-IDF v5.5.2.
- ESP-IDF v5.5.2 và espressif/esp32-camera 2.1.9; đối chiếu versions và
  dependencies.lock sau khi triển khai.
- Chọn cổng nạp và cổng console theo thiết bị Windows đang liệt kê; không
  mặc định COM4/COM5 vì cổng có thể đổi khi cắm lại.
- Firmware mới xuất log ở **cổng USB-UART** (chip CH343), theo cấu hình console
  UART trong plan. Cổng USB Serial/JTAG từng là COM4 chỉ dùng cho log firmware cũ.

## Chạy thử sau khi firmware mới được triển khai

1. Trong terminal ESP-IDF v5.5.2 ở gốc repo, chạy `idf.py set-target esp32s3`,
   `idf.py build`, rồi `idf.py -p <PORT_NAP> flash`.
2. Mở console trên `<PORT_LOG>` ở 115200 baud bằng `idf.py -p <PORT_LOG> monitor`
   nếu cùng cổng nạp, hoặc serial monitor tương đương nếu khác cổng.
3. Kiểm tra thứ tự BOOT_INFO → PSRAM_TEST PASS → CAMERA_INIT thành công.
   Lưu nguyên giá trị `esp_psram_get_size()` theo byte ở BOOT_INFO và đối
   chiếu với 8 388 608 byte; nếu khác, dừng để cập nhật plan theo log board.
   Sensor phải là OV3660, khung 320×240. Theo dõi cửa sổ **125 giây** sau
   khi camera sẵn sàng: ít nhất 24 báo cáo mỗi 5 giây, valid >0, fps >0
   và invalid được ghi rõ.
4. Dùng cơ chế mô phỏng lỗi có kiểm soát ở giai đoạn triển khai để thử thiếu
   PSRAM, sai dữ liệu PSRAM, lỗi init và lỗi frame. Không rút cáp FFC khi
   board đang cấp điện. Kiểm tra mã lỗi và trạng thái FAULT báo mỗi 5 giây,
   không reboot trong ít nhất 60 giây sau lỗi cuối.
5. Thử một lỗi khung đơn lẻ, 3 lỗi liên tiếp, không có khung trong 5 giây,
   phục hồi thành công/thất bại, ngân sách 3 lượt và cấp lại sau 60 giây.
   Đối chiếu các trường Serial theo [contracts/serial.md](contracts/serial.md).
   Dùng build thử có cờ `CONFIG_BRINGUP_FAULT_INJECTION`; xác nhận cờ tắt
   trong build thường. Trên PC thử biên 4 999/5 000 và 59 999/60 000 ms bằng
   đồng hồ giả; trên board chỉ cần một hoặc hai ca 60 giây thật. Có ca riêng
   mà supervisor đạt hạn 5 giây khi `fb_get` vẫn đang chặn.

Lưu log và số liệu thật trong reports/001-camera/ khi triển khai. Log của
firmware cũ trong thư mục feature chỉ xác minh bảng chân ở 240×240.
