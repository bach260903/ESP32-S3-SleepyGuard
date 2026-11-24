# Tasks: Còi, LED và máy trạng thái cảnh báo

**Input**: `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/serial.md`, `quickstart.md`.
**Tests**: Test PC viết trước logic; log và quan sát board ở từng checkpoint.
**Organization**: T001 là phép thử chân bắt buộc trước mọi lựa chọn chân chính thức; sau đó là nền tảng, US1, US2, US3 và kiểm tra cuối.

## Phase 1: Setup — gate phần cứng

- [ ] T001 Tạo firmware probe hữu hạn trong `main/pin_probe.c` và cờ bench ở `main/Kconfig.projbuild`; thử bật/tắt từng chân LED và còi ứng viên riêng lẻ, log `PIN_PROBE` vào `reports/002-alert/01-pin-probe.log`, ghi **quan sát thực tế** sáng/tắt và kêu/im, nguồn 3,3 V, loại còi, tầng đệm và cực tính tại `reports/002-alert/01-pin-probe-check.md`. Không kích còi trần không đệm; nếu thiếu tầng đệm thì dừng phần còi. Đọc checkpoint trước T002.
- [ ] T002 Chỉ dựa vào log và quan sát T001 để chốt bảng chân/cực tính/driver tại `main/board_pins.h` và `reports/002-alert/01-pin-probe-check.md`; thử reboot lạnh và khởi động camera với dây còn gắn, đặc biệt nếu GPIO3 được chọn; lưu log. Không dùng ảnh nhãn hay giá trị GPIO2/GPIO3 cũ làm kết luận.

**Checkpoint**: T001 phải có bằng chứng âm/ánh sáng của người quan sát; T002 không được đánh dấu xong chỉ bằng dòng Serial.

---

## Phase 2: Foundational — kiểu dữ liệu và cấu hình

- [ ] T003 Định nghĩa một nguồn ngưỡng ở `main/bringup_config.h` theo `data-model.md` cho `AlertConfig.suspect_ms`, `AlertConfig.alarm_ms`, `AlertConfig.fault_repeat_ms`, `AlertConfig.tick_ms`; thêm kiểu và API thuần tại `components/alert_logic/include/alert_logic.h` theo `data-model.md` cho `Observation.kind`, `Observation.at_ms`, `Observation.source`.
- [ ] T004 Nối `components/alert_logic/CMakeLists.txt`, `components/board_io/CMakeLists.txt`, `main/CMakeLists.txt` và `tests/host/run_tests.ps1` cho component/test mới; build khung và chạy lại test feature 001, ghi `reports/002-alert/02-foundation-check.md`.

**Checkpoint**: Build và test cũ xanh, chưa có đường còi chạy tự động.

---

## Phase 3: User Story 1 — cảnh báo hai mức (P1, MVP)

**Goal**: MỞ → NHẮM → NGHI NGỜ → BÁO ĐỘNG, tắt ngay khi MỞ/ACK; còi và LED có mẫu phân biệt.
**Independent Test**: Chuỗi mốc 749/750/1499/1500 ms trên PC và lệnh giả lập trên board; quan sát còi/LED.

- [ ] T005 [US1] Viết test assertion trước logic trong `tests/host/test_alert_logic.c` cho ngưỡng, nhắm liên tục, mắt mở, ACK và thời gian nhảy lùi; chạy để chứng minh test rớt tại assertion hành vi mới, lưu `reports/002-alert/03-us1-red.txt`.
- [ ] T006 [US1] Cài logic thuần trong `components/alert_logic/alert_logic.c` theo `data-model.md` cho `AlertState.phase`, `AlertState.closed_since_ms`, `AlertState.muted_episode`, `AlertState.entered_ms` và ba trường `Observation.kind`, `Observation.at_ms`, `Observation.source`; chạy test PC xanh.
- [ ] T007 [US1] Cài bộ phát tick không chặn trong `components/board_io/alert_output.c` và `components/board_io/include/alert_output.h` theo `data-model.md` cho `OutputPattern.buzzer_on`, `OutputPattern.led_on`, `OutputPattern.next_due_ms`; dùng driver active on/off hoặc PWM âm tần **chỉ sau khi T001 chứng minh mạch đệm**. Không dùng delay để tạo bíp.
- [ ] T008 [US1] Nối lệnh `EYE OPEN`, `EYE CLOSED`, `ALERT ACK` theo `contracts/serial.md` vào `main/bringup_main.c`; lệnh không chặn queue camera và nút vật lý chỉ bật nếu T001/T002 đã kiểm chứng chân; build/nạp và thu log/quan sát tại `reports/002-alert/03-us1-check.md`.

**Checkpoint**: NGHI NGỜ 0,75 s, BÁO ĐỘNG 1,5 s, tắt ≤100 ms, FPS camera không bị chặn.

---

## Phase 4: User Story 2 — lỗi camera (P2)

**Goal**: Mẫu lỗi riêng ngay khi chốt lỗi; tiếp tục sau hết lượt, có mã và nguyên nhân; phục hồi xóa mốc nhắm cũ.
**Independent Test**: Inject lỗi giả lập và lỗi camera thật, quan sát mẫu ba bíp/LED cùng log 5 giây; phục hồi về TỈNH.

- [ ] T009 [US2] Viết test PC rớt trước logic ở `tests/host/test_alert_logic.c` cho ưu tiên lỗi, ACK không tắt LED/log, FAULT kéo dài và phục hồi cần khung hợp lệ; lưu `reports/002-alert/04-us2-red.txt`.
- [ ] T010 [US2] Cài nhánh lỗi trong `components/alert_logic/alert_logic.c` theo `data-model.md` cho `AlertState.fault_code`, `AlertState.last_cause`, `AlertState.phase` và reset `AlertState.closed_since_ms`; thêm nhịp ba bíp/triple LED trong `components/board_io/alert_output.c`; chạy test xanh.
- [ ] T011 [US2] Nối sự kiện lỗi/chốt phục hồi của supervisor camera tại `main/bringup_main.c` vào alert controller; không để `STOPPED` muộn hoặc lệnh mắt giả lập tự xóa lỗi thật; in `ALERT_FAULT code=... last_cause=...` mỗi 5 giây theo `contracts/serial.md`.
- [ ] T012 [US2] Build/nạp, thu kịch bản `CAMERA FAULT` và ít nhất một kịch bản mất khung camera thật qua cờ thử riêng; lưu log, sdkconfig, thời gian và quan sát tại `reports/002-alert/04-us2-check.md`, rồi nạp lại build bình thường.

**Checkpoint**: Lỗi riêng bắt đầu ≤100 ms, lặp 5 s ±200 ms, kể cả khi camera FAULT; không reboot, không lẫn báo ngủ gật.

---

## Phase 5: User Story 3 — không thấy mặt và đầu vào thử (P3)

**Goal**: KHÔNG THẤY MẶT là trạng thái riêng; lệnh sai không đổi trạng thái; LED đủ năm mẫu.
**Independent Test**: Gửi chuỗi lệnh hợp lệ/sai và quan sát trạng thái, LED, âm thanh cùng log.

- [ ] T013 [US3] Viết test PC rớt trước logic trong `tests/host/test_alert_logic.c` cho NO_FACE, CLOSED mới sau NO_FACE, lệnh sai và thứ tự sự kiện cùng mốc; lưu `reports/002-alert/05-us3-red.txt`.
- [ ] T014 [US3] Cài NO_FACE và kiểm tra đầu vào trong `components/alert_logic/alert_logic.c`, `main/bringup_main.c` theo `data-model.md` cho `AlertState.phase`, `AlertState.closed_since_ms` và `Observation.kind`; hoàn thiện parser các lệnh `FACE LOST`, `CAMERA RECOVERED` và phản hồi từ chối theo `contracts/serial.md`.
- [ ] T015 [US3] Hoàn thiện năm mẫu LED trong `components/board_io/alert_output.c`; build/nạp, gửi toàn bộ chuỗi `contracts/serial.md`, ghi log cùng quan sát LED/âm thanh tại `reports/002-alert/05-us3-check.md`.

**Checkpoint**: Không thấy mặt khác lỗi camera, không suy thành mắt mở hay nhắm, không cảnh báo sai.

---

## Phase 6: Polish & cross-cutting

- [ ] T016 Tắt chế độ probe/fault injection trong `sdkconfig.defaults` và build thường; nạp bản thường, chạy 125 giây với đầu vào MỞ, kiểm tra 24 báo cáo camera, `invalid=0`, không cảnh báo ngoài ý muốn. Thêm ca Serial không được đọc hoặc nhận lệnh dồn dập trong khi phát cảnh báo để chứng minh output tick và queue camera không bị chặn bởi log/lệnh; không cần microSD. Lưu `reports/002-alert/06-final-125s.log` và `reports/002-alert/06-final-check.md`.
- [ ] T017 Cập nhật loại còi, bảng chân có bằng chứng, giới hạn bàn thử và mẫu lỗi trong `docs/DECISIONS.md`, `README.md`, `specs/002-buzzer-state-controller/quickstart.md`; kiểm tra không có ảnh/video mặt trong staged diff và không đưa `testdata/` vào Git.
- [ ] T018 Đối chiếu FR/SC với log và test trong `reports/002-alert/06-final-check.md`, chạy `tests/host/run_tests.ps1`, `git diff --check`; chỉ commit/push các file feature 002 đã chọn, không dùng `git add .`.

---

## Dependencies & execution order

- T001 → T002 là gate vật lý; nếu còi không đệm thì không chạy bước cấp tín hiệu còi.
- T002 → T003/T004 → US1 → US2 → US3 → Phase 6. Trong mỗi US, test đỏ trước logic, logic trước board.
- Không chạy các phase implementation song song trên cùng board. Tài liệu `research.md`, `data-model.md`, `contracts/serial.md` đã hoàn tất trước mã.
- **Parallel opportunities**: đọc log PC và rà tài liệu có thể làm cùng lúc với build độc lập; các task thay đổi `main/bringup_main.c` hoặc dùng board phải nối tiếp. Không có task [P] trên đường chính vì cùng tài nguyên board/firmware.

## Implementation strategy

MVP = T001–T008 (US1). Tại mỗi checkpoint đọc log và ghi quan sát trước phase tiếp theo. US2 hoàn thiện cảnh báo lỗi camera; US3 hoàn thiện tình huống không thấy mặt. Phase 6 mới nạp bản thường và chốt tài liệu.
