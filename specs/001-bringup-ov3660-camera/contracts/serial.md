# Serial output contract

Giao diện quan sát của firmware mới là console UART qua cổng USB-UART/CH343.
Số COM do Windows cấp có thể thay đổi. Firmware cũ từng ghi log qua USB
Serial/JTAG COM4; không dùng cổng đó để kết luận firmware mới im lặng.
Không xuất dữ liệu ảnh. Mỗi dòng có timestamp, cấp độ, mã sự kiện và các
trường tên=giá trị để người thử dễ đọc và máy có thể đối chiếu.

## Sự kiện tối thiểu

| Sự kiện | Trường bắt buộc |
|---|---|
| BOOT_INFO | psram_size_raw_bytes (`esp_psram_get_size()`), psram_expected_bytes, psram_heap_free_bytes, flash_physical, flash_expected |
| PSRAM_TEST | test_bytes, patterns, result |
| CAMERA_INIT | attempt, result, sensor nếu thành công, raw_error nếu lỗi |
| FRAME_REPORT | width, height, valid, invalid, elapsed_ms, fps |
| FRAME_UNAVAILABLE | stage, reason, raw_error nếu có |
| RECOVERY | stage=start/success/failure/exhausted, remaining, reason |
| RECOVERY_WAIT | phase=stopping, waited_ms, budget_remaining; ngay khi bắt đầu chờ dừng và mỗi 5 giây, kể cả khi worker đang chặn |
| FAULT | code, reason, remaining; lặp mỗi 5 giây ở trạng thái cuối |

Các mã lỗi chuẩn là PSRAM_UNAVAILABLE, PSRAM_CONFIG_MISMATCH,
PSRAM_TEST_FAILED, CAM_INIT_FAILED, FRAME_UNAVAILABLE, RECOVERY_EXHAUSTED.
`WORKER_STOP_TIMEOUT` là mã lỗi bổ sung khi worker không phát sự kiện kết thúc
trong 15 000 ms; supervisor không deinit hoặc reboot khi báo mã này.
Mỗi lượt phục hồi có đúng một sự kiện kết quả cuối và số lượt còn lại. Lỗi đơn
lẻ được tính trong FRAME_REPORT dù chưa kích hoạt phục hồi.
