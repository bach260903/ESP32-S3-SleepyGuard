# Feature 002 bench validation

This feature is a bench prototype. It does not detect eyes from camera images and has not been validated for driving.

## Prerequisites

- ESP32-S3 CAM N16R8 with OV3660, USB power, ESP-IDF v5.5.2 and esp32-camera 2.1.9.
- Còi và LED đã được nối theo sơ đồ thực tế. Trước khi bật bất kỳ chân còi nào, xác nhận mạch có tầng đệm và hoạt động ở 3,3 V. Nếu chỉ có loa/còi hai dây trần, dừng phép thử còi cho đến khi có mạch đệm phù hợp.
- Xác định cổng COM hiện tại; COM4/COM5 chỉ là các lần đo trước.

## 1. Chốt chân bằng phép thử đầu tiên

1. Build/nạp chế độ pin probe hữu hạn theo T001. Mỗi lần chỉ cấp một chân ứng viên; giữ các chân khác ở trạng thái an toàn. Không thử các chân camera, flash, PSRAM và USB.
2. Thu `reports/002-alert/01-pin-probe.log` với `tools/capture_serial.py`.
3. Quan sát bằng mắt và tai khi từng chân bật/tắt. Điền vào `01-pin-probe-check.md`: ứng viên, mức điện, loại mạch, LED sáng/tắt, còi kêu/im, cực tính, thời điểm quan sát và kết luận.
4. Chỉ sau khi log **và** quan sát khớp, chốt bảng chân và chạy lại khởi động camera. Nếu còi thụ động có đệm, thử âm tần ngắn; nếu module active có đệm, thử nhịp bật/tắt. Không suy từ ảnh hoặc nhãn chân.

## 2. Logic PC

Chạy `tests/host/run_tests.ps1`. Kiểm tra mốc 749/750/1499/1500 ms, mắt mở, ACK, không thấy mặt, lỗi camera, phục hồi và thời gian nhảy lùi. Test PC phải rớt tại assertion mới trước khi thêm logic và xanh sau triển khai.

## 3. Board theo từng checkpoint

Build và flash từng phase. Dùng `python tools/capture_serial.py --port COMx --seconds 125 --output reports/002-alert/local.log` hoặc `--usb-jtag` nếu console đang ở USB Serial/JTAG. Đọc `contracts/serial.md` để phát lệnh và đối chiếu output.

| Ca | Lệnh / kích thích | Quan sát |
| --- | --- | --- |
| Hai mức | `EYE OPEN`, `EYE CLOSED` | NGHI NGỜ tại 0,75 s, BÁO ĐỘNG tại 1,5 s; âm và LED khác nhau |
| Tắt | `ALERT ACK` hoặc nút đã chứng minh, rồi `EYE OPEN` | còi tắt ≤100 ms, không tự bật lại trong cùng đợt |
| Không thấy mặt | `FACE LOST` | trạng thái/LED riêng, còi ngủ gật tắt |
| Lỗi camera | `CAMERA FAULT`; thêm ca lỗi thật | mẫu lỗi ba bíp, log mỗi 5 s; sau `CAMERA RECOVERED` và khung hợp lệ thì về TỈNH |
| Bản thường | `EYE OPEN`, chạy 125 s | camera tiếp tục báo khung hợp lệ, không cảnh báo ngoài ý muốn |

Thử thêm khi không có terminal đọc Serial và khi lệnh giả lập đến dồn dập:
bộ phát vẫn đúng nhịp, queue camera vẫn được phục vụ. Chỉ dùng log metadata
sau khi kết nối lại để đối chiếu; không dựa vào việc in log thành công để
quyết định bật/tắt còi.

Không dùng `git add .` khi lưu bằng chứng. Chỉ đưa mã, tài liệu và log metadata đã đọc vào commit. Ảnh/video mặt để thử AI đặt dưới `testdata/` đã được ignore; không commit hoặc push chúng.
