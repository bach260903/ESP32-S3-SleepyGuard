# Constitution – ESP32-S3 Cảnh báo tài xế ngủ gật

Mỗi nguyên tắc dưới đây xuất phát từ một quyết định đã **Accepted** trong
`docs/DECISIONS.md` (mã D-xxx ghi kèm). Không thêm nguyên tắc nào chưa có quyết định.

## Nguyên tắc cốt lõi

### I. Cảnh báo độc lập với tính năng phụ (D-015)
Đường cảnh báo camera → phát hiện → còi phải chạy đúng khi tính năng phụ (ghi log,
microSD…) lỗi hoặc vắng mặt. Tính năng phụ không được chặn hay làm chậm vòng lặp
phát hiện.

### II. Lỗi camera phải được xử lý tường minh (D-007)
Khởi tạo thất bại, mất khung hình hoặc hình bất thường không được bỏ qua trong
im lặng. Thiết bị báo cho tài xế bằng mẫu bíp riêng, thử khởi tạo lại có giới hạn,
và nếu không phục hồi thì tiếp tục báo định kỳ. Các thông số cụ thể (số lần thử,
khoảng chờ, mẫu bíp) được chốt trong spec của feature liên quan.

### III. Kiểm thử trên board thật (D-008)
Logic thuần (máy trạng thái, bộ đếm) tách riêng để kiểm thử trên PC. Mọi thứ chạm tới
camera, còi hoặc thời gian phản hồi phải có bằng chứng chạy trên ESP32-S3 N16R8 +
OV3660 thật trước khi coi là xong.

### IV. Ngưỡng cấu hình ở một nơi (D-005, D-006)
Thời gian nhắm mắt tối đa X (mặc định 1,5 giây) và các ngưỡng khác đặt ở một nơi
cấu hình duy nhất, không hard-code rải rác. Báo động có hai mức: bíp nhẹ rồi còi
lớn; còi tắt khi mắt mở lại hoặc khi bấm nút.

### V. Xử lý tại thiết bị, không lưu hình ảnh (D-002, D-009)
Mọi xử lý diễn ra trên thiết bị. Không lưu ảnh hay video khuôn mặt, không gửi dữ liệu
ra ngoài. Muốn đổi phải có quyết định mới trong `docs/DECISIONS.md` trước.

### VI. Giữ phạm vi và nói rõ giới hạn (D-010, D-011, D-012)
Phiên bản 1 chỉ hỗ trợ ban ngày và cấp nguồn qua USB. Tài liệu phải nêu rõ đây là hệ
thống hỗ trợ, không thay thế việc nghỉ ngơi khi lái xe, và chưa được kiểm chứng để
dùng trên đường thật.

### VII. Lát cắt nhỏ và ghi chép quyết định (D-003, D-014)
Mỗi feature là một lát cắt nhỏ, kiểm tra được trên board và có hướng dẫn chạy từng
bước. Phiên bản ESP-IDF được ghim và ghi trong `docs/DECISIONS.md`. Quyết định kiến
trúc mới phải được ghi vào file đó.

## Quản trị

Constitution được cập nhật khi `docs/DECISIONS.md` có quyết định mới hoặc thay đổi.
Khi hai nơi mâu thuẫn, `docs/DECISIONS.md` là nguồn sự thật và constitution phải sửa theo.

**Phiên bản:** 1.0.0 · **Ngày thông qua:** 2026-11-19
