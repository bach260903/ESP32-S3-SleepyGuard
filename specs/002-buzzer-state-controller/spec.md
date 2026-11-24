# Feature Specification: Bộ điều khiển cảnh báo còi và LED

**Feature Branch**: `main`
**Created**: 2026-11-24
**Status**: Draft
**Input**: Thêm máy trạng thái cảnh báo tài xế, còi và LED với đầu vào mắt giả lập để kiểm thử trước khi có nhận diện mắt; báo riêng lỗi camera, giữ đường cảnh báo độc lập với chức năng phụ. Chân và loại còi phải xác nhận bằng thử nghiệm trên board.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Cảnh báo hai mức khi mắt nhắm (Priority: P1)

Người thử gửi sự kiện mắt mở/nhắm giả lập. Thiết bị phân biệt tỉnh, nghi ngờ và báo động bằng còi, LED và dòng trạng thái. Khi mắt mở lại hoặc có thao tác tắt cảnh báo, còi dừng.

**Why this priority**: Đây là đường cảnh báo cốt lõi và có thể kiểm thử độc lập trước khi tích hợp bộ nhận diện mắt.

**Independent Test**: Trên PC phát chuỗi sự kiện có mốc thời gian xác định; trên board phát cùng chuỗi và quan sát còi, LED, log. Không cần mô hình AI.

**Acceptance Scenarios**:

1. **Given** mắt đang mở, **When** bắt đầu nhắm liên tục, **Then** trạng thái chuyển từ TỈNH sang NGHI NGỜ sau 0,75 giây và phát bíp ngắn thưa.
2. **Given** mắt vẫn nhắm, **When** đủ 1,5 giây kể từ lúc bắt đầu nhắm, **Then** chuyển sang BÁO ĐỘNG với mẫu còi dồn dập hơn.
3. **Given** đang nghi ngờ hoặc báo động, **When** nhận mắt mở, **Then** còi tắt trong tối đa 100 ms, LED về chỉ báo tỉnh và bộ đếm nhắm được xóa.
4. **Given** đang phát còi, **When** người dùng nhấn nút tắt hoặc phát sự kiện nhấn nút giả lập, **Then** còi tắt trong tối đa 100 ms và không phát lại cho cùng đợt nhắm liên tục; mắt mở cho phép đợt mới.

---

### User Story 2 - Báo lỗi camera riêng (Priority: P2)

Khi không có khung hình hợp lệ, tài xế cần biết thiết bị tạm thời không giám sát được. Mẫu còi và LED lỗi camera phải khác cảnh báo ngủ gật, bắt đầu khi lỗi được xác nhận và tiếp diễn khi lỗi chưa hết.

**Why this priority**: Bring-up camera cho thấy một đợt mất khung thông thường tạo khoảng gần 6 giây không giám sát được.

**Independent Test**: Giả lập lỗi và phục hồi camera, sau đó kích hoạt lỗi thật theo kịch bản chẩn đoán; đối chiếu log và quan sát mẫu lỗi.

**Acceptance Scenarios**:

1. **Given** đang ở bất kỳ trạng thái nào, **When** camera được xác nhận lỗi, **Then** chuyển sang LỖI CAMERA, ngừng mẫu báo ngủ gật và bắt đầu mẫu lỗi riêng trong tối đa 100 ms.
2. **Given** camera vẫn lỗi và chưa có thao tác tắt, **When** qua mỗi chu kỳ 5 giây, **Then** mẫu lỗi lặp lại và log vẫn nêu mã lỗi cùng nguyên nhân gần nhất, kể cả khi đã hết lượt phục hồi; sau thao tác tắt, LED và log vẫn lặp nhưng còi im.
3. **Given** camera vừa phục hồi, **When** có khung hợp lệ mới, **Then** ngừng mẫu lỗi và trở về TỈNH; thời gian nhắm cũ không được dùng tiếp.

---

### User Story 3 - Xử lý khi không thấy mặt và quan sát trạng thái (Priority: P3)

Khi đầu vào báo không thấy mặt, thiết bị không được coi đó là mắt mở hay nhắm. LED và log phải giúp người thử phân biệt tình huống này với lỗi camera. Đầu vào giả lập cho phép thử toàn bộ chuyển trạng thái mà không ghi hoặc gửi hình ảnh.

**Why this priority**: Tách tình huống không thấy mặt khỏi lỗi camera và báo động giảm cảnh báo sai khi tích hợp nhận diện sau này.

**Independent Test**: Gửi các lệnh giả lập mắt mở, mắt nhắm, không thấy mặt, lỗi/phục hồi camera và tắt còi; kiểm tra trạng thái, mẫu LED/còi và thời điểm chuyển.

**Acceptance Scenarios**:

1. **Given** camera hoạt động, **When** nhận KHÔNG THẤY MẶT, **Then** chuyển sang trạng thái riêng, xóa bộ đếm nhắm, tắt mẫu báo ngủ gật và hiển thị mẫu LED riêng.
2. **Given** đang KHÔNG THẤY MẶT, **When** lại nhận mắt nhắm, **Then** bắt đầu đếm một đợt nhắm mới từ mốc đầu vào đó.
3. **Given** lệnh giả lập không hợp lệ, **When** xử lý lệnh, **Then** log từ chối lệnh nhưng không đổi trạng thái hay kích còi.

### Edge Cases

- Đồng hồ thời gian không tiến hoặc nhảy lùi: không được rút ngắn ngưỡng cảnh báo.
- Sự kiện mắt mở đến sát ngưỡng phải thắng trước khi phát bíp mới; cùng mốc thời gian dùng thứ tự nhận sự kiện.
- Lỗi camera có ưu tiên cao hơn mọi trạng thái mắt; sự kiện mắt khi đang lỗi không được tạo báo động ngủ gật.
- Một lần bấm tắt chỉ áp dụng cho đợt báo hiện tại; lỗi camera vẫn phải tiếp tục được ghi log định kỳ.
- Mất mặt không được tính là mắt mở để xác nhận tài xế tỉnh, cũng không được tiếp tục bộ đếm nhắm.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Hệ thống MUST nhận đầu vào giả lập mắt mở, mắt nhắm, không thấy mặt, lỗi/phục hồi camera và thao tác tắt cảnh báo bằng giao diện thử nghiệm có phản hồi lỗi rõ ràng.
- **FR-002**: Hệ thống MUST giữ các trạng thái TỈNH, NGHI NGỜ, BÁO ĐỘNG, LỖI CAMERA và KHÔNG THẤY MẶT; mỗi chuyển trạng thái MUST ghi trạng thái trước, sau, lý do và mốc thời gian.
- **FR-003**: Hệ thống MUST đo thời gian mắt nhắm liên tục; ngưỡng khởi đầu NGHI NGỜ là 0,75 giây và BÁO ĐỘNG là 1,5 giây, đặt tại một nơi cấu hình.
- **FR-004**: Hệ thống MUST phát hai mẫu còi khác nhau cho NGHI NGỜ và BÁO ĐỘNG; mẫu BÁO ĐỘNG phải dồn dập hơn. Không đòi thay đổi âm lượng vật lý nếu còi chỉ hỗ trợ bật/tắt.
- **FR-005**: Hệ thống MUST tắt còi khi mắt mở lại hoặc có thao tác tắt; thao tác tắt MUST không làm phát lại còi trong cùng đợt nhắm liên tục.
- **FR-006**: Hệ thống MUST dùng mẫu còi và LED riêng cho LỖI CAMERA, lặp lại mỗi 5 giây khi lỗi còn tồn tại, và ghi mã lỗi cùng nguyên nhân gần nhất định kỳ; thao tác tắt chỉ làm im còi của đợt lỗi hiện tại.
- **FR-007**: Hệ thống MUST ưu tiên lỗi camera, không dùng trạng thái mắt cũ sau khi camera phục hồi, và bắt đầu báo lỗi từ lúc lỗi được xác nhận thay vì đợi phục hồi xong.
- **FR-008**: Hệ thống MUST hiển thị LED khác nhau cho TỈNH, NGHI NGỜ, BÁO ĐỘNG, LỖI CAMERA và KHÔNG THẤY MẶT; các mẫu phải được mô tả để người thử quan sát được.
- **FR-009**: Hệ thống MUST xác nhận chân LED, chân còi, cực tính và loại mạch còi bằng phép thử từng chân trên board kèm log và quan sát thực tế trước khi chốt bảng chân; MUST NOT cấp dòng trực tiếp từ chân điều khiển cho còi không có mạch đệm.
- **FR-010**: Logic trạng thái và thời gian MUST kiểm thử được trên PC độc lập với phần cứng; thay đổi điều khiển còi, LED hoặc thời gian phản hồi MUST có bằng chứng chạy trên ESP32-S3 N16R8 với OV3660.
- **FR-011**: Đường cảnh báo MUST tiếp tục hoạt động khi chức năng phụ như lưu log hoặc microSD vắng mặt hay lỗi; xử lý lệnh thử và ghi log MUST không chặn vòng cảnh báo.
- **FR-012**: Hệ thống MUST không lưu hoặc truyền ảnh/video khuôn mặt và MUST chỉ chạy trên bàn thử trong feature này; nhận diện mắt thật, dùng trên đường, ban đêm và nguồn 12 V không thuộc phạm vi.

### Key Entities

- **Đầu vào quan sát**: loại sự kiện, thời điểm và nguồn; đầu vào mắt trong feature này là giả lập.
- **Đợt nhắm mắt**: mốc bắt đầu và trạng thái đã tắt cảnh báo; kết thúc khi mắt mở, mất mặt hoặc camera lỗi.
- **Trạng thái cảnh báo**: tên trạng thái, thời điểm vào trạng thái và lý do chuyển.
- **Mẫu cảnh báo**: chu kỳ bật/tắt còi và LED gắn với trạng thái, gồm mẫu lỗi camera riêng.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Với đầu vào giả lập nhắm liên tục, 100% ca kiểm thử chuyển NGHI NGỜ ở mốc 0,75 giây và BÁO ĐỘNG ở mốc 1,5 giây, sai số quan sát trên board không quá 100 ms.
- **SC-002**: Trong 100% ca mắt mở hoặc tắt cảnh báo, còi dừng trong 100 ms và không phát lại cho cùng đợt nhắm.
- **SC-003**: Trong 100% ca lỗi camera không bị tắt bởi người dùng, mẫu lỗi riêng bắt đầu trong 100 ms sau khi lỗi được xác nhận và lặp mỗi 5 giây với sai số không quá 200 ms cho đến khi lỗi được xóa; sau thao tác tắt, log và LED vẫn lặp.
- **SC-004**: Người thử phân biệt được đủ năm trạng thái bằng LED và log theo bảng mẫu; lỗi camera và không thấy mặt không dùng cùng mẫu.
- **SC-005**: Ít nhất một ca lỗi camera thật trên board và toàn bộ chuỗi chuyển trạng thái giả lập có log thời gian, kết quả quan sát còi/LED; logic PC qua tất cả ca biên đã liệt kê.
- **SC-006**: Trong ca thử 125 giây với đầu vào mắt mở và chức năng phụ không dùng, camera tiếp tục có báo cáo khung hợp lệ, không xuất hiện lỗi khung hay báo động ngoài ý muốn.

## Assumptions

- Người dùng đã xác nhận còi và LED phát/sáng trong một phép thử trước đó; loại mạch còi, cực tính và chân thực tế vẫn phải xác nhận bằng phép thử có quan sát rồi mới chốt.
- Nếu còi là loại thụ động hoặc loa hai dây, việc tạo âm tần và mạch đệm an toàn được quyết định sau kiểm tra phần cứng; các mẫu trạng thái ở đây chỉ quy định nhịp và ý nghĩa cảnh báo.
- Bản đầu dùng lệnh Serial làm nguồn giả lập; thao tác tắt có thể được giả lập hoặc thực hiện bằng nút đã kiểm chứng trên board.
- Ngưỡng 1,5 giây là giá trị khởi đầu theo D-005; 0,75 giây là giả định tạm để có mức NGHI NGỜ và sẽ được hiệu chuẩn khi có dữ liệu nhận diện.
- Thiết bị dùng nguồn USB và thử trên bàn, chưa được kiểm chứng khi lái xe.
