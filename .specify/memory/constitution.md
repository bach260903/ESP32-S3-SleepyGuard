<!--
Sync Impact Report
- Version change: unversioned Spec Kit template → 1.0.0 (initial adoption).
- Modified principles: template placeholders → seven project principles I–VII,
  derived from accepted decisions D-002, D-003, D-005–D-012, D-014, D-015.
- Added sections: Additional Constraints; Development Workflow.
- Removed sections: none.
- Follow-up TODOs: none.
-->

# ESP32-S3 Cảnh báo tài xế ngủ gật Constitution

Các nguyên tắc này dựa trên các quyết định đã chấp nhận trong `docs/DECISIONS.md`.
Chỉ thay đổi chúng sau khi cập nhật quyết định tương ứng.

## Core Principles

### I. Cảnh báo độc lập với tính năng phụ (D-015)

Đường cảnh báo camera → phát hiện → còi MUST hoạt động khi tính năng phụ như ghi log
hoặc microSD bị lỗi hoặc không có. Tính năng phụ MUST NOT chặn hoặc làm chậm vòng
lặp phát hiện. Đường cảnh báo là chức năng cốt lõi của thiết bị.

### II. Lỗi camera được báo tường minh (D-007)

Lỗi khởi tạo, mất khung hình và hình bất thường MUST tạo cảnh báo bằng mẫu bíp riêng.
Thiết bị MUST thử khởi tạo lại với giới hạn và MUST báo định kỳ nếu không phục hồi.
Spec của feature liên quan MUST xác định số lần thử, khoảng chờ và mẫu bíp.

### III. Kiểm thử trên board thật (D-008)

Logic thuần như máy trạng thái và bộ đếm MUST tách khỏi phần cứng để kiểm thử trên PC.
Thay đổi liên quan camera, còi hoặc thời gian phản hồi MUST có bằng chứng chạy trên
ESP32-S3 N16R8 với OV3660 thật trước khi được coi là hoàn thành.

### IV. Ngưỡng tập trung và báo động hai mức (D-005, D-006)

Ngưỡng nhắm mắt X và các ngưỡng khác MUST được đặt tại một nơi cấu hình duy nhất.
Giá trị khởi đầu của X là 1,5 giây; giá trị cuối cùng MUST được chốt bằng thực nghiệm.
Báo động MUST có mức bíp nhẹ khi nghi ngờ và còi lớn khi vượt X. Còi MUST tắt khi
mắt mở lại hoặc khi bấm nút.

### V. Xử lý tại thiết bị, không lưu hình ảnh (D-002, D-009)

Xử lý MUST diễn ra trên thiết bị. Hệ thống MUST NOT lưu ảnh hoặc video khuôn mặt và
MUST NOT gửi dữ liệu ra ngoài. Thay đổi quy tắc này cần một quyết định mới trong
`docs/DECISIONS.md`.

### VI. Giữ phạm vi và nói rõ giới hạn (D-010, D-011, D-012)

Phiên bản 1 chỉ hỗ trợ ban ngày và cấp nguồn qua USB. Tài liệu MUST nêu rằng thiết
bị chỉ hỗ trợ, không thay thế việc nghỉ ngơi khi lái xe, và chưa được kiểm chứng để
dùng trên đường thật.

### VII. Feature nhỏ và quyết định có ghi chép (D-003, D-014)

Mỗi feature MUST là một lát cắt nhỏ, kiểm tra được trên board và có hướng dẫn chạy
từng bước. Phiên bản ESP-IDF MUST được ghim và ghi trong `docs/DECISIONS.md` khi
bring-up. Quyết định kiến trúc mới MUST được ghi trong nhật ký quyết định.

## Additional Constraints

Nền tảng của phiên bản đầu là ESP32-S3 N16R8, flash 16 MB, PSRAM octal 8 MB và
camera OV3660 (D-001). Còi dự kiến là module active buzzer có mạch đệm; GPIO
MUST NOT cấp dòng trực tiếp cho còi không có mạch đệm (D-013). Chân còi còn phải
được xác nhận trên board thật trước khi triển khai.

## Development Workflow

Dùng ESP-IDF và Spec Kit. Với mỗi feature, thực hiện theo thứ tự constitution →
specify → clarify → plan → tasks → implement (D-003, D-014). Spec của feature MUST
đưa ra tiêu chí kiểm tra được, nhất là với hành vi lỗi và phần cứng. Việc kiểm tra
hoàn thành MUST đối chiếu kết quả với spec, các quyết định liên quan và bằng chứng
trên board theo Nguyên tắc III.

## Governance

`docs/DECISIONS.md` là nguồn sự thật cho các quyết định đã chấp nhận. Khi quyết
định mới được chấp nhận hoặc quyết định cũ thay đổi, cập nhật constitution và ghi
lý do, tác động trước khi áp dụng cho feature tiếp theo. Nếu hai tệp mâu thuẫn,
MUST sửa constitution theo quyết định đã chấp nhận.

Phiên bản constitution tuân theo semantic versioning: MAJOR cho thay đổi hoặc bỏ
nguyên tắc không tương thích; MINOR cho nguyên tắc hay hướng dẫn quản trị mới;
PATCH cho làm rõ câu chữ không đổi nghĩa. Khi rà soát mỗi feature, MUST kiểm tra
spec, plan, tasks và kết quả thực hiện có tuân thủ các nguyên tắc liên quan hay
không; ngoại lệ cần một quyết định mới được chấp nhận.

**Version**: 1.0.0 | **Ratified**: 2026-11-19 | **Last Amended**: 2026-11-19
