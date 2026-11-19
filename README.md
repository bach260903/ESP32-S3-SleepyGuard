# ESP32-S3 – Cảnh báo tài xế ngủ gật

> **Trạng thái: ý tưởng / khởi đầu.** Chưa có mã nguồn chạy được.
> Mọi quyết định nằm trong [`docs/DECISIONS.md`](docs/DECISIONS.md).

## Mục tiêu

Một thiết bị nhỏ đặt trong xe. Camera quan sát khuôn mặt tài xế; nếu mắt
**nhắm liên tục quá X giây** thì thiết bị phát còi cảnh báo.
Toàn bộ xử lý diễn ra **trên thiết bị**, không cần mạng.

Giá trị X dự kiến mặc định là 1,5 giây và sẽ được chỉnh bằng thực nghiệm.

## Lưu ý an toàn

Đây là dự án thử nghiệm, **chỉ hỗ trợ, không thay thế** việc nghỉ ngơi khi
lái xe. Nếu buồn ngủ, hãy dừng xe ở nơi an toàn. Chưa được kiểm chứng để
dùng trên đường thật.

## Phần cứng

| Thành phần | Chi tiết |
|---|---|
| Vi điều khiển | ESP32-S3 module N16R8 (16 MB flash, 8 MB PSRAM octal) |
| Camera | OV3660 |
| Còi | Còi chủ động (active buzzer) dạng module có sẵn mạch đệm – dự kiến, cần xác nhận khi có phần cứng |

Chưa xác nhận: board có khe microSD hay không, và chân GPIO nào còn trống
cho còi (sẽ đối chiếu sơ đồ chân khi bring-up).

## Cách hoạt động (dự kiến)

```
Camera → phát hiện mặt → cắt vùng mắt → phân loại mắt mở/nhắm
       → bộ đếm thời gian nhắm → máy trạng thái → còi
```

Các trạng thái dự kiến: `TỈNH` → `NGHI NGỜ` (bíp nhẹ) → `BÁO ĐỘNG` (còi lớn),
cùng hai trạng thái đặc biệt `LỖI CAMERA` và `KHÔNG THẤY MẶT`.

## Phạm vi phiên bản 1

- Có: phát hiện nhắm mắt ban ngày, báo động hai mức, xử lý lỗi camera rõ ràng.
- Không có: hoạt động ban đêm (cần camera NoIR và đèn hồng ngoại), lưu ảnh/video,
  kết nối mạng, nguồn 12 V trên xe (giai đoạn đầu cấp nguồn qua USB).

## Lộ trình (mỗi mục là một feature nhỏ trong Spec Kit)

1. Bring-up camera: khởi tạo OV3660, lấy khung hình, báo lỗi rõ ràng.
2. Còi + máy trạng thái với đầu vào giả lập (nút bấm / lệnh Serial).
3. Spike nhận diện mắt mở/nhắm: đo FPS và độ chính xác trên board.
4. Ghép nhận diện vào máy trạng thái và còi.
5. Hiệu chuẩn theo từng người, xử lý "không thấy mặt".
6. Ghi log chuyển trạng thái (Serial, sau đó microSD nếu board hỗ trợ).

## Công cụ

- Framework: ESP-IDF (cài qua tiện ích Espressif IDF cho VS Code).
- Quy trình: Spec Kit (`constitution → specify → clarify → plan → tasks → implement`).

## Cấu trúc thư mục (dự kiến)

```
README.md
docs/DECISIONS.md      # nhật ký quyết định
.specify/              # Spec Kit (constitution, template)
specs/                 # mỗi feature một thư mục
main/, components/     # mã nguồn ESP-IDF (tạo khi bắt đầu feature 001)
```
