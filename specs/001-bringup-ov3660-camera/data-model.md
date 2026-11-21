# Data Model: camera bring-up

Feature này chỉ giữ dữ liệu tạm trong RAM; không có lưu trữ ảnh.

## Configuration

Một cấu hình ứng dụng chứa độ phân giải 320×240, pixel format RGB565, 3 lần
khởi tạo ban đầu, 3 lượt phục hồi, ngưỡng 3 khung lỗi liên tiếp, 5 giây mất
khung, 5 giây chờ khung đầu sau khởi tạo lại, 60 giây ổn định, chu kỳ báo
thống kê 5 giây và báo lỗi cuối 5 giây. Worker chờ 20 ms sau khi `fb_get`
trả NULL để tránh quay vòng bận; tham số `BRINGUP_CAMERA_NULL_RETRY_MS` nằm
trong `main/bringup_config.h`. Mỗi ngưỡng có tên riêng theo ý nghĩa.
Các trường cấu hình có ràng buộc riêng: `Configuration.BRINGUP_CAMERA_NULL_RETRY_MS`
= 20 ms; `Configuration.BRINGUP_STOP_WAIT_LIMIT_MS` = 15 000 ms tính từ lúc
supervisor yêu cầu dừng; `Configuration.BRINGUP_RECOVERY_WAIT_PERIOD_MS` =
5 000 ms, độc lập với chu kỳ báo FAULT và FRAME_REPORT.
Cấu hình còn có PSRAM kỳ vọng **8 MiB = 8 388 608 byte** và kích thước vùng
thử tối thiểu **153 600 byte** cho RGB565 320×240. GPIO vận hành nằm trong
bảng của [plan.md](plan.md).

## Camera State

Trạng thái: INIT → RUNNING → RECOVERING → RUNNING hoặc FAULT. Các pha con khi
phục hồi là STOPPING → STARTING → WAITING_FIRST_FRAME; khung hợp lệ đầu tiên
đưa máy trạng thái trở lại RUNNING. Từ INIT cũng
có thể vào FAULT khi PSRAM hoặc camera thất bại. FAULT là trạng thái cuối,
không còn thử camera và không tự reboot. Mỗi trạng thái có thời điểm bắt đầu,
mã lỗi gần nhất và nguyên nhân.

## Frame Observation

Mỗi lần lấy khung có thời điểm, kết quả hợp lệ/không hợp lệ và kích thước.
Khung chỉ hợp lệ khi có dữ liệu và đúng 320×240. Khung được trả lại driver sau
khi xử lý; không giữ ảnh trong báo cáo hoặc log. Sự kiện phân biệt rõ `fb_get`
trả NULL (`fb_get_null`) với khung có dữ liệu nhưng sai kích thước hoặc độ dài
(`invalid_size`); cả hai được đếm là khung lỗi và ghi mã `FRAME_UNAVAILABLE`.

## Camera Worker Ownership và sự kiện đến muộn

Worker là chủ sở hữu duy nhất của driver camera. Supervisor chỉ gọi
`bringup_camera_request_stop()` khi chốt mất khung và chuyển sang giai đoạn
chờ dừng trong RECOVERING. Cờ dừng được xóa trước khi tạo worker mới, không
xóa ở đầu hàm worker để tránh mất yêu cầu dừng đến ngay sau khi tạo task.
Worker kiểm tra cờ dừng trước và sau `fb_get`, trả frame đã nhận và bỏ kết quả
đến muộn. Worker kết thúc bằng đúng một sự kiện cuối: `STOPPED` khi supervisor
yêu cầu dừng, hoặc `INIT_FAILED` khi init lỗi hoặc lỗi giả lập. Cả hai phát
sau khi worker đã trả mọi frame đang giữ và deinit nếu driver đã khởi tạo.
Supervisor không deinit. Lượt phục hồi thất bại do `INIT_FAILED` vẫn đã tiêu
hao một lượt.

Từ lúc chốt mất khung đến khi nhận sự kiện kết thúc của worker, supervisor
**vẫn rút queue** để
không chặn worker ở `send_event` và để Serial báo định kỳ; mọi sự kiện `FRAME`
và `READY` cũ đều bị bỏ, không tăng bộ đếm, không đổi trạng thái hay ngân sách.
Queue dùng chung theo FIFO: sự kiện cũ của worker đứng trước sự kiện kết thúc.
Chỉ sau `STOPPED` hoặc `INIT_FAILED` của worker trước mới được gọi
`bringup_camera_start()` cho lượt phục hồi tiếp theo. Thời gian chờ khung
phục hồi bắt đầu khi init mới hoàn tất, không tính thời gian worker cũ dừng
hoặc init mới chạy.

Trong STOPPING, supervisor phát `RECOVERY_WAIT phase=stopping waited_ms=...
budget_remaining=...` ngay khi bắt đầu và sau mỗi
`Configuration.BRINGUP_RECOVERY_WAIT_PERIOD_MS`. Nếu chưa nhận sự kiện kết thúc
sau `Configuration.BRINGUP_STOP_WAIT_LIMIT_MS`, chuyển FAULT với mã
`WORKER_STOP_TIMEOUT`; vẫn rút queue, không deinit, không khởi động lại. Sự
kiện kết thúc đến sau FAULT không được khởi động camera lại.

Mất khung khi ngân sách = 0: chốt mất khung → yêu cầu dừng → vẫn rút queue,
bỏ FRAME/READY, in RECOVERY_WAIT → nhận sự kiện kết thúc của worker → FAULT
(`RECOVERY_EXHAUSTED`); không gọi `bringup_camera_start()`. Nếu worker không
kết thúc trong 15 000 ms, FAULT dùng `WORKER_STOP_TIMEOUT` thay vì chờ mãi.

Các kịch bản giả lập frame dùng `FaultScenario.worker_index`: 0 nghĩa là mọi
worker, M > 0 là chỉ worker thứ M của phiên chạy và chỉ một lần ở worker đó.
`FaultScenario.after_first_valid_s` là K giây kể từ khung hợp lệ đầu của
worker đã chọn; mặc định 0. Kịch bản init lỗi chọn worker M nhưng không cần
đợi khung đầu. Chỉ các build bật `CONFIG_BRINGUP_FAULT_INJECTION` mới có điểm
giả lập. Thời lượng ngừng gửi frame hoặc trì hoãn worker là
`Configuration.BRINGUP_FAULT_DELAY_MS` = 6 000 ms trong cấu hình chung.
Riêng ca giữ framebuffer dùng `CONFIG_BRINGUP_FAULT_HOLD_MS` (Kconfig, chỉ có
khi bật fault injection và chọn HOLD_FRAME), mặc định 6 000 ms, phạm vi
1 000–60 000 ms. Ca chờ dài dùng 17 000 ms để thấy RECOVERY_WAIT lặp sau
5 và 10 giây kể từ stop request; ca quá hạn dùng 23 000 ms để vượt giới hạn
chờ dừng 15 000 ms tính từ stop request.

Mỗi lời gọi `fb_get` được đo thời gian thực và log ở ca thử deadline trên board;
không suy thời gian chặn thực từ timeout nội bộ 4 giây của driver. Kịch bản
worker trì hoãn 6 giây sau khi trả frame là mô phỏng chậm ở worker, **không**
được gọi là bằng chứng `fb_get` thật đã chặn quá 5 giây.

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
