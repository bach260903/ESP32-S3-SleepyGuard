# Data Model: camera bring-up

Feature này chỉ giữ dữ liệu tạm trong RAM; không có lưu trữ ảnh.

## Configuration

Một cấu hình ứng dụng chứa độ phân giải 320×240, pixel format RGB565, 3 lần
khởi tạo ban đầu, 3 lượt phục hồi, ngưỡng 3 khung lỗi liên tiếp, 5 giây mất
khung, 5 giây chờ khung đầu sau khởi tạo lại, 60 giây ổn định, chu kỳ báo
thống kê 5 giây và báo lỗi cuối 5 giây. Mỗi ngưỡng có tên riêng theo ý nghĩa.
Cấu hình còn có PSRAM kỳ vọng **8 MiB = 8 388 608 byte** và kích thước vùng
thử tối thiểu **153 600 byte** cho RGB565 320×240. GPIO vận hành nằm trong
bảng của [plan.md](plan.md).

## Camera State

Trạng thái: INIT → RUNNING → RECOVERING → RUNNING hoặc FAULT. Từ INIT cũng
có thể vào FAULT khi PSRAM hoặc camera thất bại. FAULT là trạng thái cuối,
không còn thử camera và không tự reboot. Mỗi trạng thái có thời điểm bắt đầu,
mã lỗi gần nhất và nguyên nhân.

## Frame Observation

Mỗi lần lấy khung có thời điểm, kết quả hợp lệ/không hợp lệ và kích thước.
Khung chỉ hợp lệ khi có dữ liệu và đúng 320×240. Khung được trả lại driver sau
khi xử lý; không giữ ảnh trong báo cáo hoặc log.

## Measurement Window

Một khoảng đo có thời điểm bắt đầu/kết thúc, số khung hợp lệ, số khung lỗi,
chiều rộng/cao và FPS = số khung hợp lệ / số giây thực tế. Sau phục hồi, bắt
đầu một khoảng mới; không trộn số khung trước và sau lỗi.

## Recovery Budget

Ban đầu có 3 lượt. Mỗi lần khởi tạo lại trừ đúng một lượt, dù thành công hay
thất bại. Một khung lỗi đơn lẻ không xóa ngân sách hoặc đặt lại thời gian ổn
định. Điều kiện mất khung duy nhất là 3 khung lỗi liên tiếp hoặc 5 giây không
có khung hợp lệ. Mốc 5 giây tính từ khung hợp lệ gần nhất khi đang RUNNING;
nếu chưa có khung hợp lệ, tính từ lúc vào RUNNING. Khi điều kiện này xảy ra,
hủy đồng hồ ổn định và phục hồi nếu còn lượt. Đồng hồ 60 giây bắt đầu từ khung
hợp lệ đầu tiên sau phục hồi; đủ
60 giây không có sự kiện mất khung thì cấp lại 3 lượt. Tất cả ngưỡng thời gian
dùng phép so sánh **>=**: mất khung tại 5 000 ms; cấp lại tại 60 000 ms,
chưa cấp tại 59 999 ms.

## PSRAM Check

Trạng thái nhận diện, dung lượng chip do driver báo, heap còn trống, kích thước vùng thử
ít nhất 153600 byte, kết quả mẫu theo địa chỉ và mẫu đảo bit. Kết quả PASS là
điều kiện cho phép khởi tạo camera. Thông tin flash vật lý chỉ là chẩn đoán.
