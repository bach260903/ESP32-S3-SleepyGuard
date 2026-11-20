# Tasks: Bring-up camera OV3660

**Input**: `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/serial.md`, `quickstart.md`.
**Scope**: ESP32-S3 CAM N16R8, OV3660, ESP-IDF v5.5.2 và `espressif/esp32-camera` 2.1.9. Chưa có nhận diện, còi, SD hoặc mạng.
**Git baseline đã hoàn tất**: repo đã `git init`, `.gitignore` bỏ qua `build/`, `managed_components/`, `sdkconfig`, `sdkconfig.old` và giữ `dependencies.lock`, `sdkconfig.defaults`; commit mốc `be6f96d`. Không lặp lại việc này như task chưa làm.
**Board gate**: Mỗi task nạp/chạy board phải lưu log Serial và bản `sdkconfig` đã sinh tại `reports/001-camera/`, đọc và xác nhận trước khi tiếp tục. Không cắm/rút FFC khi board đang có điện. Cổng log firmware mới là USB-UART/CH343, không cố định số COM.

## Phase 1: Setup — toolchain, PSRAM và camera trước máy trạng thái

**Goal**: Build khung tối thiểu có dependency đã ghim, nạp diagnostic để đọc PSRAM/flash thô, rồi thử camera QVGA. Tách build khỏi nạp để lỗi nằm đúng bước.

- [X] T001 Cài ESP-IDF v5.5.2; tạo khung rỗng tại `CMakeLists.txt`, `main/CMakeLists.txt`, `main/idf_component.yml` (ghim `espressif/esp32-camera` ==2.1.9), `main/bringup_main.c`, `sdkconfig.defaults`; chạy `idf.py set-target esp32s3` và build chưa cần board, giữ `dependencies.lock`, lưu version, lệnh và output nguyên văn tại `reports/001-camera/01-build.txt`.
- [X] T002 Tạo `tools/capture_serial.py` bằng pyserial, nhận cổng COM, baud, thời lượng và đường dẫn output, mở cổng không tương tác và ghi nguyên dòng có timestamp; thử help và lỗi tham số, ghi cách chạy tại `specs/001-bringup-ov3660-camera/quickstart.md` trước mọi phép đo board.
- [X] T003 Nạp diagnostic từ T001 qua USB-UART/CH343; trong `main/bringup_main.c` in thô `esp_psram_is_initialized()`, `esp_psram_get_size()` theo byte, kỳ vọng 8 388 608 byte, heap PSRAM còn trống và flash vật lý/kỳ vọng; thu `reports/001-camera/01-psram-flash.log` bằng T002 và lưu `reports/001-camera/01-sdkconfig.txt`. Nếu raw PSRAM khác kỳ vọng, dừng, ghi dữ liệu board vào `specs/001-bringup-ov3660-camera/plan.md`, không nới dung sai.
- [X] T004 Sau khi đọc log T003, đưa bảng chân vận hành có bằng chứng vào `main/board_pins.h`, khởi tạo OV3660 QVGA RGB565 một framebuffer PSRAM trong `main/bringup_main.c`; build/nạp, xác nhận PID và khung 320×240, lưu `reports/001-camera/02-camera-probe.log` và `reports/001-camera/02-sdkconfig.txt`. Nếu build/init lỗi, giữ nguyên lỗi để chẩn đoán trước khi viết máy trạng thái.

**Checkpoint**: Đọc log T003/T004; xác nhận IDF/component, PSRAM, flash và OV3660 320×240 trước Phase 2. `invalid header` trước lần nạp đầu là bình thường vì flash cũ đã xóa.

---

## Phase 2: Foundational — cấu hình và test PC

**Goal**: Có compiler host, smoke test chạy được và API logic thuần không phụ thuộc ESP-IDF.

- [X] T005 Đặt toàn bộ ngưỡng, số lượt, QVGA/RGB565, dung lượng PSRAM kỳ vọng và kích thước vùng thử vào `main/bringup_config.h` theo `data-model.md`; giữ ngưỡng mất khung 5 giây và chờ khung recovery 5 giây là hai tham số riêng.
- [X] T006 [P] Kiểm tra GCC host MSYS2 ở `C:/msys64/mingw64/bin/gcc.exe` (đã thấy v15.2.0 nhưng chưa nằm trong PATH); nếu không chạy được thì cài/khôi phục toolchain host; tạo `tests/host/CMakeLists.txt`, `tests/host/test_smoke.c` và `tests/host/run_tests.ps1`, biên dịch/chạy smoke test trước mọi test logic, lưu output ở `reports/001-camera/03-host-smoke.txt`; không dùng compiler Xtensa/RISC-V để chạy test PC.
- [X] T007 [P] Tạo kiểu dữ liệu, API nhận sự kiện khung và Measurement Window trong `components/bringup_logic/include/bringup_logic.h`, `components/bringup_logic/bringup_logic.c`; hàm quyết định nhận `now_ms` từ caller và không `#include` header ESP-IDF. Chưa cài nhánh INIT/FAULT hay Recovery Budget của US2/US3.
- [X] T008 [P] Tạo logger Serial và mã sự kiện tại `components/board_io/include/bringup_log.h`, `components/board_io/bringup_log.c` theo `contracts/serial.md`; BOOT_INFO có `psram_size_raw_bytes`, `psram_expected_bytes`, `psram_heap_free_bytes` và flash, không xuất dữ liệu ảnh.
- [X] T009 Tạo `components/bringup_logic/CMakeLists.txt`, `components/board_io/CMakeLists.txt`, nối `main/CMakeLists.txt`, build firmware và chạy lại `tests/host/run_tests.ps1`; lưu `reports/001-camera/03-foundation-build.txt`. Xác nhận `dependencies.lock` và `sdkconfig.defaults` được Git theo dõi.

**Checkpoint**: Smoke test host và build firmware đều chạy; T006/T007/T008 độc lập file và có thể làm song song sau T005.

---

## Phase 3: User Story 1 — luồng camera (P1, MVP)

**Goal**: PSRAM test trước camera, lấy khung liên tục và báo kích thước/FPS/khung lỗi mỗi 5 giây.
**Independent Test**: Trên board, BOOT_INFO → PSRAM_TEST PASS → CAMERA_INIT; trong 125 giây có ít nhất 24 FRAME_REPORT 320×240 với valid >0, fps >0.

- [X] T010 [US1] Cài kiểm tra PSRAM trong `components/board_io/bringup_psram.c`, `components/board_io/include/bringup_psram.h`: dùng raw size từ `esp_psram_get_size()` sau gate T003, đối chiếu dung lượng đã xác nhận trên board, cấp vùng >=153 600 byte, ghi/đọc mẫu theo địa chỉ và đảo bit, giải phóng trước camera, log mã FR-001/FR-012.
- [X] T011 [US1] Chuyển probe T004 thành camera worker sở hữu driver trong `components/board_io/bringup_camera.c`, `components/board_io/include/bringup_camera.h`; xác thực buffer/kích thước 320×240, trả mọi framebuffer, không lưu ảnh hoặc dùng SD/mạng.
- [X] T012 [US1] Tích hợp boot PSRAM → camera → RUNNING và Measurement Window 5 giây trong `main/bringup_main.c` bằng T007/T008; FPS = khung hợp lệ/thời gian thực, khung lỗi đơn lẻ cũng được đếm đúng một lần.
- [X] T013 [US1] Build/nạp US1, dùng `tools/capture_serial.py` thu 125 giây tại `reports/001-camera/04-us1-stream-125s.log`, lưu `reports/001-camera/04-sdkconfig.txt`, đọc và ghi đối chiếu SC-001/SC-002 tại `reports/001-camera/04-us1-check.md`.

**Checkpoint**: Có log board US1 được đọc và xác nhận trước khi làm US2.

---

## Phase 4: User Story 2 — chẩn đoán lỗi khởi động (P2)

**Goal**: Lỗi PSRAM/init có mã rõ, init tối đa ba lần, FAULT báo định kỳ và không reboot.
**Independent Test**: Giả lập riêng từng lỗi PSRAM và init; PSRAM lỗi không khởi tạo camera; sau ba lần init lỗi, FAULT lặp mỗi 5 giây trong ít nhất 60 giây.

- [X] T014 [US2] Viết test PC trước triển khai cho ba lần init tính cả lần đầu, FAULT, nhịp báo lỗi >=5 000 ms và không thử camera/reboot thêm trong `tests/host/test_startup_faults.c`; chạy qua `tests/host/run_tests.ps1`.
- [X] T015 [US2] Cài nhánh INIT/FAULT của máy trạng thái trong `components/bringup_logic/bringup_logic.c`, API `components/bringup_logic/include/bringup_logic.h` theo FR-004/005/014; giữ đồng hồ `now_ms` do caller truyền.
- [X] T016 [US2] Thêm `CONFIG_BRINGUP_FAULT_INJECTION` mặc định tắt và choice `BRINGUP_FAULT_SCENARIO` mặc định NONE tại `main/Kconfig.projbuild`; tạo điểm giả lập PSRAM unavailable/mismatch/allocate/readback và camera init failure trong `components/board_io/bringup_psram.c`, `components/board_io/bringup_camera.c`, chỉ biên dịch khi cờ bật.
- [X] T017 [US2] Nối tối đa ba lượt init, mã/giai đoạn/raw error và FAULT định kỳ trong `main/bringup_main.c`; chênh flash chỉ báo thông tin, không chuyển FAULT.
- [X] T018 [US2] Chạy `tests/host/run_tests.ps1`, build/nạp từng scenario US2 và thu log không tương tác bằng `tools/capture_serial.py`; lưu từng `reports/001-camera/05-us2-*.log`, bản `reports/001-camera/05-us2-*-sdkconfig.txt` và kết luận SC-003/004 ở `reports/001-camera/05-us2-check.md` sau khi đọc Serial.

**Checkpoint**: Có log board cho từng lỗi US2, bao gồm 60 giây FAULT, trước khi làm US3.

---

## Phase 5: User Story 3 — phục hồi hữu hạn (P3)

**Goal**: Một định nghĩa mất khung, lượt phục hồi hữu hạn, mốc chờ riêng và cấp lại ngân sách sau đủ 60 giây ổn định.
**Independent Test**: Test PC dùng đồng hồ giả cho các biên; board chạy scenario có log lượt còn lại, thứ tự worker/deinit và FAULT.

- [ ] T018a [US3] Khai báo giao diện phục hồi trong `components/bringup_logic/include/bringup_logic.h`: kiểu trạng thái/pha, hành động trả về và các hàm `on_frame`, `on_tick`, `on_worker_ready`, `on_worker_done`. Thêm thân hàm rỗng trả `NONE` trong `bringup_logic.c`, chạy host compiler để xác nhận test có thể link, rồi commit giao diện riêng trước T019/T020. Chưa cài quy tắc phục hồi.
- [ ] T019 [P] [US3] Viết test PC trước triển khai tại `tests/host/test_recovery_logic.c`: lỗi đơn lẻ, ba lỗi liên tiếp, mốc không khung từ khung hợp lệ gần nhất hoặc lúc vào RUNNING, 4 999/5 000 ms, 59 999/60 000 ms, tiêu hao lượt đúng một lần, kết quả recovery đúng một lần; đổi chờ khung recovery thành 3 000 ms vẫn giữ ngưỡng mất khung RUNNING 5 000 ms, đổi chu kỳ báo FAULT vẫn giữ chu kỳ FRAME_REPORT. Chạy và lưu kết quả test rớt trước T021.
- [ ] T020 [P] [US3] Viết test PC cho quyết định deadline thuần trong `tests/host/test_deadline_decision.c`: supervisor chốt đúng một lần ở >=5 000 ms, bỏ FRAME/READY đến muộn từ lúc chốt mất khung đến khi nhận sự kiện kết thúc worker (`STOPPED` hoặc `INIT_FAILED`), chỉ cho phép khởi tạo lượt mới sau sự kiện đó. Test cả mất khung khi ngân sách = 0, RECOVERY_WAIT ở mốc 5 giây, và `WORKER_STOP_TIMEOUT` tại 14 999/15 000 ms khi worker không kết thúc; sau FAULT vẫn bỏ sự kiện đến muộn. Chỉ gọi `bringup_logic`, không biên dịch `bringup_camera.c`/`esp32-camera` trên PC; lưu kết quả test rớt bằng assertion trước T021.
- [ ] T021 [US3] Cài điều kiện mất khung dùng chung, ngân sách, thời gian chờ sau init lại và quyết định deadline/giai đoạn STOPPING → STARTING → WAITING_FIRST_FRAME → RUNNING trong `components/bringup_logic/bringup_logic.c`, `components/bringup_logic/include/bringup_logic.h` theo FR-006/007/009/010/011 và `data-model.md` mục Camera Worker Ownership; hàm nhận `now_ms` từ caller. STOPPING vượt `data-model.md` `Configuration.BRINGUP_STOP_WAIT_LIMIT_MS` thì FAULT `WORKER_STOP_TIMEOUT` nhưng vẫn rút queue. Chạy lại T019/T020 đến khi qua.
- [ ] T022 [US3] Thêm `bringup_camera_request_stop()` và sự kiện `BRINGUP_CAMERA_STOPPED` ở `components/board_io/include/bringup_camera.h` và `bringup_camera.c`; dùng cờ dừng atomic, xóa cờ trước khi tạo task mới chứ không xóa ở đầu worker. Worker kiểm tra cờ trước/sau `fb_get`, trả frame rồi bỏ kết quả đến muộn, tự `esp_camera_deinit()` và phát đúng một sự kiện cuối STOPPED; đường init lỗi cleanup trước khi phát đúng một INIT_FAILED. Worker chỉ deinit ở đường dừng và đường init lỗi; supervisor không deinit và chỉ khởi tạo lượt mới sau sự kiện kết thúc worker. Thêm nghỉ sau `fb_get` NULL theo `data-model.md` `Configuration.BRINGUP_CAMERA_NULL_RETRY_MS`; sự kiện phân biệt `fb_get_null` và `invalid_size`.
- [ ] T023 [US3] Mở rộng choice `BRINGUP_FAULT_SCENARIO` dưới cờ T016 tại `main/Kconfig.projbuild`, `components/board_io/bringup_camera.c`; mỗi scenario là build riêng, log tên scenario và không làm hỏng camera thật:
  - Thêm hai tham số Kconfig `data-model.md` `FaultScenario.worker_index` (0 = mọi worker, M > 0 = chỉ worker M một lần) và `FaultScenario.after_first_valid_s` (K giây từ khung hợp lệ đầu tiên của worker đó); mặc định lần lượt 1 và 0. Đếm worker trong phiên chạy riêng với `attempt` khởi tạo và lượt phục hồi. Các kịch bản frame chỉ kích hoạt từ mốc K; kịch bản init lỗi không cần khung đầu.
  - Một khung lỗi hoặc ba khung lỗi liên tiếp: gọi `fb_get` thật, trả frame, rồi gán `valid=false` cho đúng số khung được chọn.
  - Ngừng có khung: gọi `fb_get` thật, trả frame, rồi không gửi FRAME trong `data-model.md` `Configuration.BRINGUP_FAULT_DELAY_MS`.
  - Worker trễ quá hạn: trả frame thật rồi trì hoãn `Configuration.BRINGUP_FAULT_DELAY_MS` trước vòng đọc tiếp; đây là mô phỏng worker chậm, không khẳng định `fb_get` thật chặn cùng thời gian.
  - Giữ framebuffer: gọi `fb_get` thật, giữ frame trong `Configuration.BRINGUP_FAULT_DELAY_MS`, log deadline/stop request; sau đó `fb_return`, log `fb_returned`, bỏ kết quả đến muộn, deinit và log `deinit_done`, rồi phát STOPPED. Không deinit khi còn giữ frame.
  - Init lại thất bại ở lượt phục hồi N: đếm lượt phục hồi riêng với attempt khởi động, thực hiện init driver thật rồi chủ động cho lượt N thất bại và cleanup trong worker; phát INIT_FAILED sau cleanup để supervisor có thể bắt đầu lượt kế tiếp.
  - Init xong không có khung: init driver thật, phát READY rồi không gửi FRAME trong 6 giây.
- [ ] T024 [US3] Tích hợp RECOVERING/RUNNING/FAULT, RECOVERY remaining và reset khoảng FPS sau phục hồi trong `main/bringup_main.c`; từ lúc chốt mất khung đến sự kiện kết thúc worker vẫn rút queue và bỏ FRAME/READY cũ, in RECOVERY_WAIT theo `data-model.md` `Configuration.BRINGUP_RECOVERY_WAIT_PERIOD_MS` (ngay lúc bắt đầu và mỗi 5 giây), không gọi init mới trước sự kiện kết thúc. Khi ngân sách = 0: yêu cầu dừng, chờ sự kiện kết thúc rồi FAULT `RECOVERY_EXHAUSTED`; nếu quá `Configuration.BRINGUP_STOP_WAIT_LIMIT_MS` thì FAULT `WORKER_STOP_TIMEOUT`, không deinit và vẫn rút queue. Đồng hồ ổn định tính từ khung hợp lệ đầu sau phục hồi; lỗi đơn lẻ không reset đồng hồ hay xóa thống kê lỗi.
- [ ] T025 [US3] Trên board chạy riêng scenario giữ framebuffer của T023, thu `reports/001-camera/06-us3-held-frame.log` và `06-us3-held-frame-sdkconfig.txt`; log deadline → stop request → `fb_returned` → bỏ kết quả muộn → `deinit_done` → STOPPED → init lại, chứng minh chỉ worker sở hữu driver và Serial vẫn báo RECOVERY_WAIT mỗi 5 giây. Đo và log thời gian chặn thực của mỗi `fb_get` trong kịch bản; ca worker trễ sau khi đã trả frame được đo riêng, không gọi đó là `fb_get` thật bị chặn. Không tác động cáp FFC.
- [ ] T026 [US3] Chạy `tests/host/run_tests.ps1`, lưu `reports/001-camera/06-us3-host-tests.txt`; build/nạp từng scenario US3, gồm một hoặc hai ca 60 giây thật, thu `reports/001-camera/06-us3-*.log` và `reports/001-camera/06-us3-*-sdkconfig.txt`, đọc log đối chiếu SC-005/006/007 tại `reports/001-camera/06-us3-check.md`. Với mỗi ca phục hồi thành công, xác nhận khung hợp lệ thật và FRAME_REPORT/FPS tiếp tục sau deinit → init; không đặt ngưỡng FPS tối thiểu cho feature này.

**Checkpoint**: Đọc log PC và từng log board US3 trước Phase 6.

---

## Phase 6: Polish & cross-cutting

**Goal**: Bản build thường không chứa mã giả lập, có log xác nhận cuối và tài liệu đã cập nhật.

- [ ] T027 Tắt `CONFIG_BRINGUP_FAULT_INJECTION`, chọn NONE trong `sdkconfig.defaults`/`sdkconfig`; build sạch, kiểm tra bằng build output/map rằng mã scenario không biên dịch vào bản thường; lưu `reports/001-camera/07-production-build.txt` và `reports/001-camera/07-sdkconfig.txt`.
- [ ] T028 Nạp bản thường, chạy `tools/capture_serial.py` trong 125 giây, lưu `reports/001-camera/07-final-125s.log`, `reports/001-camera/07-final-sdkconfig.txt`, đọc và đối chiếu FR/SC tại `reports/001-camera/07-final-check.md`; xác nhận không xuất ảnh, SD, mạng hoặc còi.
- [ ] T029 Cập nhật `README.md`, `specs/001-bringup-ov3660-camera/quickstart.md` và `docs/DECISIONS.md`: ghi cách build/nạp/thu log, giới hạn chỉ dùng trên bàn thử, phiên bản IDF/component đã xác nhận bằng build ở D-003 và bằng chứng pin/board ở D-001; đối chiếu phạm vi Serial-only đã chốt ở D-007 và constitution v2.0.0 trước khi coi feature hoàn tất.

---

## Dependencies & execution order

- Setup: T001 → T002 → T003 → T004. T003 là gate PSRAM thô; dừng khi raw size khác kỳ vọng để cập nhật plan theo log, không nới dung sai.
- Foundation: T005 → T006/T007/T008 (khác file, có thể làm song song) → T009. T006 kiểm compiler host và smoke test trước T014/T019/T020.
- US1: T010 → T011 → T012 → T013. Đọc log T013 trước US2.
- US2: T014 → T015; T016 có thể chuẩn bị song song với T014 vì khác file; T015/T016 → T017 → T018. Đọc log T018 trước US3.
- US3: T018a khai báo API/thân rỗng và commit → T019/T020 là test PC phải rớt bằng assertion trước T021 → T021 → T022 → T023 → T024 → T025 → T026. T025 kiểm worker trên board và đo thời gian `fb_get` thật; T020 chỉ kiểm quyết định logic trên PC.
- Polish: T027 → T028 → T029. Mỗi build scenario board có log và bản sdkconfig riêng; nếu `idf.py build` hoặc flash lỗi, giữ nguyên thông báo để chẩn đoán trước khi sửa.

## Parallel examples

- Foundation: T006 dựng test host, T007 API logic và T008 logger chạm các file khác nhau.
- US2: T014 test PC viết trước implementation T015, trong khi T016 chuẩn bị Kconfig ở file riêng.
- US3: T019/T020 test PC độc lập với board; không nạp đồng thời nhiều scenario lên một board.

## Implementation strategy

Làm từng phase, đọc log và tự xác nhận sau mỗi checkpoint. MVP là T001–T013 (US1); chỉ tiếp US2 sau log US1, chỉ tiếp US3 sau log US2. Board đang xóa flash nên `invalid header` trước lần nạp mới là bình thường. Nếu flash không kết nối, giữ BOOT, nhấn/nhả RST rồi nhả BOOT và thử lại qua USB-UART/CH343; nếu vẫn lỗi, giữ nguyên thông báo lỗi.
