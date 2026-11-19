# Feature Specification: Bring-up camera OV3660

**Feature Branch**: Không tạo; dự án chưa bật git hook của Spec Kit

**Created**: 2026-11-19

**Status**: Draft

**Input**: Bring-up OV3660 trên ESP32-S3 N16R8; xác nhận camera và PSRAM, đo kích thước
khung hình cùng FPS qua Serial; báo lỗi có mã, thử phục hồi hữu hạn và dừng ở trạng
thái lỗi xác định. Chưa gồm nhận diện, còi hoặc thẻ nhớ.

## Clarifications

### Session 2026-11-19

- Q: Giới hạn 3 lần khởi tạo lại được cấp lại khi nào? → A: Chọn C: chỉ cấp
  lại sau 60 giây chạy ổn định liên tục. 60 giây là mặc định, đặt tại cấu hình
  chung theo D-005. Khi hết lượt và không phục hồi được, dừng ở trạng thái lỗi
  nhưng vẫn in lỗi định kỳ qua Serial theo D-007 và constitution II.
- Q: Khung lỗi đơn lẻ có đặt lại thời gian ổn định không? → A: Chọn B: chỉ
  đặt lại khi đạt định nghĩa mất khung hình dùng chung (3 lỗi liên tiếp hoặc
  5 giây không có khung hợp lệ). Khung lỗi đơn lẻ vẫn được đếm và báo mỗi
  5 giây; mỗi sự kiện phục hồi ghi số lượt còn lại. Thời gian ổn định tính
  từ khung hợp lệ đầu tiên sau lần phục hồi gần nhất.
- Q: Khi nào một lượt phục hồi được coi là thành công? → A: Chọn A: khởi tạo
  thành công và nhận khung hợp lệ trong 5 giây tính từ lúc khởi tạo xong.
  Lượt thất bại vẫn tiêu hao một lượt. Thời gian chờ là tham số riêng trong
  cấu hình chung, mặc định 5 giây, độc lập với ngưỡng mất khung hình. Mỗi lượt
  in kết quả thành công/thất bại và số lượt còn lại qua Serial.
- Q: Cần bằng chứng nào để xác nhận PSRAM hoạt động? → A: Chọn A: xác nhận
  dung lượng phù hợp cấu hình N16R8, phân biệt dung lượng chip do driver báo
  với heap còn trống. Sau khi in giá trị thô trên board và xác nhận ý nghĩa
  của API, đối chiếu dung lượng chip với 8 MiB = 8 388 608 byte; không nới
  dung sai nếu giá trị khác. Trước khi khởi tạo camera,
  kiểm tra ghi–đọc nhiều mẫu trên vùng nhớ ít nhất cỡ một khung hình, không
  quét toàn bộ PSRAM. Thất bại thì dừng lỗi và báo định kỳ, không tự khởi động
  lại. In thêm dung lượng flash với kỳ vọng 16 MB, chỉ để đối chiếu thông tin.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Xác nhận luồng camera hoạt động (Priority: P1)

Là người bring-up board, tôi cấp nguồn và xem Serial để biết PSRAM sẵn sàng,
camera tạo khung hình liên tục ở độ phân giải thấp, kích thước thực tế và tốc độ
khung hình đo được. Điều này xác nhận nền tảng phần cứng trước khi thêm nhận diện.

**Why this priority**: Không có luồng khung hình ổn định thì các feature sau không
có dữ liệu đầu vào.

**Independent Test**: Khởi động board với camera kết nối, theo dõi Serial trong
125 giây sau khi camera sẵn sàng và kiểm tra thông tin PSRAM, kích thước
khung hình và FPS định kỳ.

**Acceptance Scenarios**:

1. **Given** board và OV3660 hoạt động, **When** cấp nguồn, **Then** Serial báo
   dung lượng PSRAM và flash, kết quả kiểm tra PSRAM đạt trước khi bắt đầu
   khởi tạo camera, camera sẵn sàng và bắt đầu nhận khung hình 320 × 240.
2. **Given** camera đang tạo khung hình, **When** theo dõi Serial trong cửa sổ
   125 giây sau khi camera sẵn sàng,
   **Then** mỗi khoảng báo cáo 5 giây có chiều rộng 320, chiều cao 240, số khung
   hợp lệ, số khung lỗi trong khoảng và FPS đo được lớn hơn 0.

---

### User Story 2 - Chẩn đoán lỗi khi khởi động (Priority: P2)

Là người bring-up board, tôi cần biết rõ camera hoặc PSRAM hỏng ở bước nào,
không phải suy đoán từ việc thiết bị im lặng hoặc khởi động lại liên tục.

**Why this priority**: Lỗi khởi động có thể che khuất lỗi đấu dây và cấu hình.

**Independent Test**: Tạo riêng tình huống PSRAM không sẵn sàng và camera không
khởi tạo được; quan sát mã lỗi, số lần thử và trạng thái cuối trên Serial.

**Acceptance Scenarios**:

1. **Given** PSRAM không sẵn sàng, **When** khởi động, **Then** Serial ghi mã
   `PSRAM_UNAVAILABLE` và thiết bị vào trạng thái lỗi xác định, báo lỗi định kỳ
   và không khởi tạo camera.
2. **Given** camera không khởi tạo được, **When** khởi động, **Then** Serial ghi
   `CAM_INIT_FAILED` cùng mã lỗi gốc nếu có, thử tối đa 3 lần khởi tạo tính cả
   lần đầu, rồi ghi `RECOVERY_EXHAUSTED` và vào trạng thái lỗi xác định.
3. **Given** PSRAM không phù hợp cấu hình N16R8 hoặc kiểm tra ghi–đọc thất bại,
   **When** kiểm tra trước khởi tạo camera, **Then** ghi mã lỗi bộ nhớ tương ứng,
   dừng lỗi và báo định kỳ; không khởi tạo camera hoặc tự khởi động lại.
4. **Given** dung lượng flash báo về khác kỳ vọng 16 MB, **When** ghi thông tin
   khởi động, **Then** hiển thị cả giá trị nhận diện và kỳ vọng; feature này
   không chuyển sang trạng thái lỗi chỉ vì chênh lệch đó.

---

### User Story 3 - Phục hồi hữu hạn khi mất khung hình (Priority: P3)

Là người bring-up board, tôi muốn thấy rõ việc mất khung hình sau khi camera đã
chạy và biết thiết bị phục hồi hay dừng lỗi, thay vì treo không có thông tin.

**Why this priority**: Mất khung hình khi đang chạy là lỗi có thể xảy ra trên
board thật và cần được phân biệt với khởi tạo thất bại.

**Independent Test**: Gây lỗi lấy khung hình sau khi đã có báo cáo FPS; kiểm tra
thông báo lỗi, giới hạn thử lại và một trong hai kết quả cuối.

**Acceptance Scenarios**:

1. **Given** camera đã chạy, **When** một lần lấy khung hình thất bại hoặc khung
   không hợp lệ, **Then** Serial ghi `FRAME_UNAVAILABLE` cùng chi tiết lỗi nếu có.
2. **Given** đạt điều kiện mất khung hình ở FR-006, **When** kích hoạt phục hồi,
   **Then** hệ thống thử khởi tạo lại và ghi số lượt còn lại tại mỗi sự kiện
   trong số lượt còn lại của ngân sách 3 lần; nếu thành công thì tiếp tục báo
   cáo kích thước và FPS, chỉ cấp lại đủ 3 lượt sau 60 giây chạy ổn định liên tục.
3. **Given** mọi lần phục hồi đều thất bại, **When** hết giới hạn, **Then** Serial
   ghi `RECOVERY_EXHAUSTED`, thiết bị vào trạng thái lỗi xác định và không tự
   khởi động lại hoặc thử camera thêm; lỗi vẫn được in định kỳ qua Serial.
4. **Given** camera phục hồi rồi lỗi lại trước 60 giây ổn định, **When** cần
   phục hồi tiếp, **Then** chỉ dùng số lượt còn lại; không được cấp lại 3 lượt.
5. **Given** đang tính thời gian ổn định từ khung hợp lệ đầu tiên sau phục hồi,
   **When** có lỗi đơn lẻ nhưng chưa đạt FR-006, **Then** thời gian vẫn tiếp tục,
   báo cáo 5 giây ghi đủ số khung lỗi và đủ 60 giây thì cấp lại 3 lượt.
6. **Given** một lượt phục hồi đã khởi tạo camera xong, **When** nhận khung hợp
   lệ trong thời gian chờ cho phép, **Then** báo lượt đó thành công và số lượt
   còn lại; bắt đầu tính thời gian ổn định từ khung hợp lệ này.
7. **Given** khởi tạo lại thất bại hoặc hết thời gian chờ mà chưa có khung hợp
   lệ, **When** kết thúc lượt thử, **Then** ghi thất bại và số lượt còn lại,
   không hoàn lại lượt đã tiêu hao; thử tiếp nếu còn lượt, nếu không thì dừng lỗi.

### Edge Cases

- Khung hình có dữ liệu nhưng kích thước khác 320 × 240 hoặc chiều rộng/chiều cao
  bằng 0: coi là khung không hợp lệ và ghi lỗi.
- Một khung lỗi đơn lẻ rồi khung tiếp theo hợp lệ: giữ trạng thái chạy, đặt lại
  bộ đếm lỗi liên tiếp và tiếp tục thống kê FPS; không đặt lại thời gian ổn định
  hoặc xóa số khung lỗi đã tích lũy trong khoảng báo cáo hiện tại.
- Camera phục hồi giữa các lần thử: báo đã phục hồi và bắt đầu lại khoảng đo FPS;
  không dùng số khung cũ để tính tốc độ mới.
- Lượt phục hồi cuối thành công: tiếp tục chạy với 0 lượt dự phòng; nếu lỗi cần
  phục hồi tái diễn trước khi đủ 60 giây ổn định thì chuyển sang lỗi xác định.
- Khởi tạo lại thành công nhưng chưa có khung hợp lệ: vẫn đang phục hồi,
  không báo chạy lại và không tính thời gian ổn định cho đến khung hợp lệ đầu tiên.
- Serial không có máy tính theo dõi: việc ghi thông tin không được làm camera
  dừng hoặc tự khởi động lại.
- Heap PSRAM còn trống thấp hơn dung lượng chip do driver báo vì hệ thống đã
  dùng một phần: không kết luận sai cấu hình chỉ từ heap còn trống.
- Không cấp được vùng nhớ thử đủ lớn: báo kiểm tra PSRAM thất bại và dừng lỗi,
  không bỏ qua kiểm tra hoặc giảm vùng thử xuống dưới một khung hình.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Trước khi khởi tạo camera, thiết bị MUST ghi dung lượng PSRAM
  nhận diện được, phân biệt dung lượng chip do driver báo và heap còn trống,
  rồi xác nhận phù hợp cấu hình N16R8 với PSRAM danh nghĩa 8 MiB. Không dùng
  số byte heap còn trống làm tiêu chí. Cách đối chiếu dung lượng MUST được ghi rõ trong
  kế hoạch theo ý nghĩa của số liệu mà nền tảng báo, không được chấp nhận cấu
  hình dung lượng nhỏ hơn chỉ bằng cách nới dung sai. PSRAM không sẵn sàng MUST
  báo `PSRAM_UNAVAILABLE`; cấu hình không phù hợp MUST báo `PSRAM_CONFIG_MISMATCH`.
  Cả hai trường hợp MUST dừng lỗi, báo định kỳ và không khởi tạo camera.
- **FR-002**: Thiết bị MUST khởi tạo OV3660 để nhận khung hình 320 × 240; trạng
  thái thành công MUST được ghi qua Serial trước khi báo cáo FPS.
- **FR-003**: Khi camera sẵn sàng, thiết bị MUST lấy khung hình liên tục và cứ
  5 giây MUST ghi chiều rộng, chiều cao, số khung hợp lệ, số khung lỗi và FPS
  đo được cho khoảng đó. Số khung lỗi MUST tính mọi lần lấy khung thất bại hoặc
  nhận khung không hợp lệ, kể cả lỗi đơn lẻ, mỗi lần chỉ đếm một lần. FPS MUST
  được tính từ số khung hợp lệ và thời gian đo thực tế.
- **FR-004**: Mỗi lỗi khởi tạo hoặc lấy khung hình MUST được ghi qua Serial với
  mã lỗi ổn định, giai đoạn xảy ra lỗi và mã lỗi gốc nếu có. Các mã tối thiểu là
  `PSRAM_UNAVAILABLE`, `PSRAM_CONFIG_MISMATCH`, `PSRAM_TEST_FAILED`,
  `CAM_INIT_FAILED`, `FRAME_UNAVAILABLE` và
  `RECOVERY_EXHAUSTED`.
- **FR-005**: Khi camera không khởi tạo được, thiết bị MUST thử tối đa 3 lần tính
  cả lần đầu. Mỗi lần thử MUST có thông báo số thứ tự; hết giới hạn MUST vào
  trạng thái lỗi xác định.
- **FR-006**: Định nghĩa duy nhất của **mất khung hình** là: có ít nhất 3 lần
  lấy khung liên tiếp thất bại/không hợp lệ HOẶC đã đủ 5 giây không nhận khung
  hợp lệ. Cùng điều kiện này MUST được dùng cho xác định mất khung hình,
  kích hoạt phục hồi nếu còn lượt và đặt lại bộ đếm ổn định; không có điều kiện
  riêng cho từng hành vi. Một khung lỗi chưa đạt điều kiện MUST vẫn được báo.
  Ngân sách phục hồi ban đầu là 3 lượt; mỗi lần khởi tạo lại tiêu tốn một lượt,
  kể cả khi thành công. Chỉ sau 60 giây chạy ổn định liên tục MUST cấp lại đủ
  3 lượt. Khung hợp lệ tiếp theo MUST xóa chuỗi lỗi liên tiếp, không tự cấp lại lượt.
  Khoảng 60 giây MUST bắt đầu tại khung hợp lệ đầu tiên sau lần phục hồi gần
  nhất. Lỗi đơn lẻ chưa đạt điều kiện mất khung hình MUST NOT đặt lại khoảng
  này. Khi đạt điều kiện, hủy khoảng đang tính và chỉ bắt đầu khoảng mới từ
  khung hợp lệ đầu tiên sau lần phục hồi tiếp theo.
- **FR-007**: Khi phục hồi thành công, thiết bị MUST báo trạng thái chạy lại và
  bắt đầu khoảng đo FPS mới. Khi thất bại hết giới hạn, thiết bị MUST ghi
  `RECOVERY_EXHAUSTED`, dừng lấy khung hình và không tự khởi động lại. Nếu lỗi
  cần phục hồi tái diễn khi không còn lượt, thiết bị MUST vào cùng trạng thái
  lỗi xác định. Trong trạng thái này MUST không thử camera thêm và MUST tiếp
  tục in mã lỗi cùng nguyên nhân định kỳ qua Serial.
  Mỗi sự kiện bắt đầu, thành công, thất bại hoặc hết lượt phục hồi MUST ghi
  số lượt còn lại; khi bắt đầu thử, số được ghi là số còn lại sau khi trừ lượt đó.
- **FR-008**: Feature này MUST không thực hiện nhận diện khuôn mặt/mắt, phát còi,
  ghi thẻ nhớ, lưu ảnh/video hoặc truyền dữ liệu ra mạng.
- **FR-009**: Thời gian ổn định để cấp lại lượt MUST có mặc định 60 giây và đặt
  tại một nơi cấu hình chung cùng các ngưỡng khác theo D-005.
- **FR-010**: Mỗi lượt phục hồi MUST chỉ được xác nhận thành công khi khởi tạo
  camera thành công và nhận được khung hợp lệ trong thời gian chờ mặc định
  5 giây tính từ thời điểm khởi tạo xong; thời gian khởi tạo không nằm trong
  khoảng chờ này. Khởi tạo thất bại hoặc hết thời gian chờ mà không có khung
  hợp lệ MUST kết thúc lượt đó với kết quả thất bại. Mỗi lượt chỉ tiêu hao
  một lượt ngân sách khi bắt đầu; thất bại không được hoàn lượt hoặc trừ thêm.
  Mỗi lượt MUST ghi đúng một kết quả cuối thành công/thất bại cùng số lượt còn
  lại qua Serial. Nếu thất bại thì thử tiếp khi còn lượt, nếu không thì vào lỗi
  xác định. Điều kiện mất khung FR-006 chỉ áp dụng khi đang chạy; khi đang
  phục hồi, việc chờ khung đầu tiên dùng điều kiện riêng tại FR-010.
- **FR-011**: Thời gian chờ khung đầu tiên sau khởi tạo lại MUST là tham số
  riêng trong cấu hình chung theo D-005, mặc định 5 giây. Tham số này độc lập
  với ngưỡng không có khung hợp lệ của FR-006 dù có cùng giá trị mặc định.
- **FR-012**: Sau bước xác nhận cấu hình PSRAM và trước khi khởi tạo camera,
  thiết bị MUST kiểm tra ghi–đọc trên một vùng PSRAM có kích thước ít nhất bằng
  dung lượng dữ liệu một khung 320 × 240 ở định dạng được chọn trong kế hoạch.
  Kiểm tra MUST dùng ít nhất hai mẫu bổ sung cho nhau, gồm mẫu phụ thuộc địa
  chỉ và mẫu đảo bit tương ứng; không yêu cầu quét toàn bộ 8 MB. Serial MUST
  ghi kích thước vùng thử, các mẫu đã kiểm tra và kết quả. Không cấp được vùng
  thử hoặc dữ liệu đọc sai MUST báo `PSRAM_TEST_FAILED`, dừng ở trạng thái lỗi
  xác định, báo định kỳ và không khởi tạo camera hoặc tự khởi động lại.
- **FR-013**: Khi khởi động, thiết bị MUST in dung lượng flash nhận diện được
  và kỳ vọng danh nghĩa 16 MB. Đây chỉ là thông tin chẩn đoán; chênh lệch dung
  lượng flash không phải điều kiện thất bại của feature này.
- **FR-014**: Trong mọi trạng thái lỗi xác định, thiết bị MUST ghi mã lỗi và
  nguyên nhân ngay khi vào trạng thái, sau đó lặp lại mỗi 5 giây mặc định.
  Chu kỳ báo lỗi MUST là tham số riêng trong cấu hình chung. Việc báo định kỳ
  không được kích hoạt lại camera, chạy lại kiểm tra PSRAM hoặc khởi động lại board.

### Key Entities

- **Khung hình hợp lệ**: Một khung camera có dữ liệu và kích thước 320 × 240;
  chỉ được đếm trong khoảng đo hiện tại, không được lưu sau khi xử lý.
- **Báo cáo đo**: Kích thước khung, số khung hợp lệ, số khung lỗi, thời lượng đo và FPS của
  một khoảng 5 giây.
- **Sự kiện lỗi**: Mã lỗi ổn định, giai đoạn, số lần thử, số lượt phục hồi còn
  lại khi là sự kiện phục hồi, và mã lỗi gốc nếu có.
- **Trạng thái camera**: Đang khởi tạo, đang chạy, đang phục hồi hoặc lỗi xác định.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Trên board thật với camera hoạt động, người kiểm thử xác nhận được
  cấu hình PSRAM phù hợp, kết quả kiểm tra ghi–đọc đạt, dung lượng flash và kích
  thước 320 × 240 từ Serial sau một lần khởi động. Thứ tự báo cáo phải chứng minh
  kiểm tra PSRAM hoàn tất trước khi bắt đầu khởi tạo camera.
- **SC-002**: Trong cửa sổ quan sát 125 giây sau khi camera sẵn sàng,
  người kiểm thử nhận ít nhất 24 báo cáo
  cách nhau khoảng 5 giây; mỗi báo cáo có số khung hợp lệ và FPS lớn hơn 0.
  Đây là tiêu chí xác nhận luồng, chưa đặt FPS tối thiểu cho nhận diện.
- **SC-003**: Với từng tình huống lỗi PSRAM, khởi tạo camera và mất khung hình,
  100% lần thử có mã lỗi phân biệt được trên Serial và kết quả cuối xác định.
  Các lỗi PSRAM gồm không sẵn sàng, sai cấu hình dung lượng, không cấp được
  vùng thử và dữ liệu đọc lại không khớp; không trường hợp nào được khởi tạo camera.
- **SC-004**: Trong thử nghiệm không phục hồi được, số lần khởi tạo camera không
  vượt giới hạn quy định; trong 60 giây sau trạng thái lỗi cuối, board không tự
  khởi động lại hay thử camera thêm lần nào, đồng thời vẫn báo lỗi định kỳ.
  Mỗi khoảng 5 giây trong trạng thái lỗi phải có báo cáo mã lỗi và nguyên nhân.
- **SC-005**: Thử lỗi tái diễn trước và sau ngưỡng ổn định xác nhận ngân sách
  không được đặt lại trước 60 giây và được cấp lại đủ 3 lượt khi đạt ngưỡng.
- **SC-006**: Thử lỗi đơn lẻ và các điều kiện biên của FR-006 xác nhận cùng
  một điều kiện quyết định mất khung, phục hồi và đặt lại thời gian ổn định.
  Tổng số khung lỗi báo cáo phải khớp số lỗi được tạo trong các khoảng đo;
  mọi sự kiện phục hồi phải có số lượt còn lại đúng với số lần đã thử.
- **SC-007**: Thử khởi tạo lại thất bại, khởi tạo xong nhưng không có khung và
  khởi tạo xong có khung hợp lệ xác nhận đúng kết quả từng lượt, đúng một lần
  tiêu hao ngân sách và đúng mốc bắt đầu thời gian chờ. Thay đổi tham số chờ
  khung phục hồi không làm thay đổi ngưỡng mất khung khi đang chạy.

## Assumptions

- Độ phân giải khởi đầu là 320 × 240 để bring-up; khả năng phù hợp cho nhận diện
  mắt sẽ được đánh giá ở feature spike sau, chưa được khẳng định tại đây.
- Giới hạn khởi tạo lúc đầu là 3 lần tính cả lần đầu. Ngân sách phục hồi là
  3 lượt, chỉ được cấp lại sau thời gian ổn định đã chốt trong Clarifications.
- Cần đối chiếu sơ đồ chân của board và phiên bản ESP-IDF khi lập kế hoạch.
- Chu kỳ báo lỗi cuối mặc định là 5 giây, dùng tham số riêng ở cấu hình chung.
  Định dạng khung, số byte của vùng thử và cách đối chiếu dung lượng PSRAM
  là chi tiết cần chốt trong kế hoạch; mục tiêu kiểm tra đã được quy định ở FR-001/012.
- Đây là firmware chẩn đoán trên bàn thử, cấp nguồn USB, chưa dùng trên đường.
  Yêu cầu bíp lỗi trong D-007 và constitution áp dụng cho thiết bị hoàn chỉnh;
  feature bring-up này chỉ ghi lỗi qua Serial vì còi thuộc feature sau.
