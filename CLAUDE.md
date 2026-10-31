# CLAUDE.md — Đồ án: Viết driver cho chuột USB trên Linux

Tài liệu chung cho cả nhóm (và cho Claude Code) khi làm việc trong repo này.
Mọi thay đổi về cấu trúc thư mục, quy ước hay môi trường phải cập nhật lại file này.

---

## 1. Đề tài

**Môn:** Hệ điều hành — Bài tập lớn
**Đề tài:** Viết driver (kernel module) cho thiết bị chuột USB trên Linux.

Driver là một **USB interface driver** (`struct usb_driver`). Nó nhận báo cáo HID
dạng *boot protocol* từ interrupt endpoint của chuột, rồi đẩy sự kiện vào
**Linux Input Subsystem** (`struct input_dev`). Ứng dụng đọc các sự kiện này qua `/dev/input/eventX`.
Phần cứng thử nghiệm là **Raspberry Pi**.

Luồng dữ liệu:

```
Chuột ──USB──► Raspberry Pi: usbcore
      ──► [driver của nhóm: probe → URB interrupt → usb_mouse_irq()]
      ──► input core ──► evdev ──► /dev/input/eventX ──► evtest / desktop (con trỏ chuột)
```

## 2. Mục tiêu

### Bắt buộc (MVP)
1. Module build được bằng Kbuild và nạp/gỡ sạch bằng `insmod`/`rmmod`, không leak, không oops.
2. Tự nhận diện chuột qua `id_table` (HID class / boot subclass / mouse protocol).
3. `probe()` cấp phát URB và buffer DMA, đăng ký `input_dev` với đủ capability
   (`EV_KEY`: BTN_LEFT/RIGHT/MIDDLE; `EV_REL`: REL_X/REL_Y/REL_WHEEL).
4. Callback URB phân tích báo cáo, gọi `input_report_*` và `input_sync`, rồi submit lại URB.
5. `open/close` của input_dev bật/tắt URB. `disconnect()` giải phóng đúng thứ tự.
6. Demo trên Pi: unbind `usbhid`, bind driver của nhóm, xem sự kiện bằng `evtest` và `dmesg`,
   và thấy con trỏ di chuyển trên desktop của Pi.

### Mở rộng (làm sau khi MVP chạy ổn)
- Nút phụ BTN_SIDE/BTN_EXTRA.
- Tham số module (`module_param`): `swap_buttons`, `invert_y`, `sensitivity`.
- Thuộc tính sysfs để đổi cấu hình lúc chạy và xem thống kê (số click, số gói tin).
- Chương trình userspace `tools/event_reader` để demo cho báo cáo.

### Ngoài phạm vi
- Parser HID report descriptor đầy đủ (report protocol). Driver chỉ dùng boot protocol.
- Chuột không dây có dongle đa chức năng, chuột gaming nhiều interface.

## 3. Cấu trúc thư mục

```
usb_mouse_driver/
├── CLAUDE.md                 # File này
├── README.md                 # Hướng dẫn nhanh: build, nạp, demo
├── Makefile                  # Wrapper: make / clean / load / unload / log
├── Kbuild                    # obj-m + danh sách object của module
├── src/                      # Mã nguồn kernel module (usb_mouse.ko)
│   ├── usb_mouse.h           #   struct usb_mouse, hằng số, prototype dùng chung
│   ├── usb_mouse_main.c      #   module_init/exit, usb_driver, id_table, module_param
│   ├── usb_mouse_probe.c     #   probe() / disconnect(): cấp phát và giải phóng tài nguyên
│   ├── usb_mouse_input.c     #   tạo và cấu hình input_dev, open()/close()
│   ├── usb_mouse_irq.c       #   callback URB: parse báo cáo → input events, resubmit
│   └── usb_mouse_sysfs.c     #   (mở rộng) thuộc tính sysfs, thống kê
├── scripts/
│   ├── setup_pi.sh           # Chạy trên Pi: cài toolchain, kernel headers, evtest
│   ├── sync_to_pi.sh         # Chạy trên WSL: rsync code sang Pi để build thử nhanh
│   ├── common.sh             # Hàm dùng chung: tìm interface chuột, driver đang gắn
│   ├── load.sh               # Chạy trên Pi: insmod, unbind usbhid, bind driver nhóm
│   │                         #   (bind lỗi thì tự trả chuột về usbhid)
│   └── unload.sh             # Chạy trên Pi: gỡ driver, trả chuột về usbhid
├── tools/
│   ├── Makefile
│   └── event_reader.c        # Userspace: đọc /dev/input/eventX, in sự kiện dễ đọc
└── docs/
    ├── report/               # Báo cáo (docx/pdf) và hình ảnh, ảnh chụp màn hình demo
    ├── reference/            # Tài liệu tham khảo (báo cáo mẫu, link)
    └── notes.md              # Ghi chép debug, kết quả thử nghiệm
```

Cách chia file bám theo các phần của một USB driver (khởi tạo, probe, truyền dữ liệu,
ngắt kết nối), nên mỗi thành viên có thể nhận một file mà ít đụng nhau.

### Kbuild (dự kiến)
```make
obj-m := usb_mouse.o
usb_mouse-y := src/usb_mouse_main.o src/usb_mouse_probe.o \
               src/usb_mouse_input.o src/usb_mouse_irq.o
# usb_mouse-y += src/usb_mouse_sysfs.o   # bật khi làm phần mở rộng
```
Trong `Makefile` dùng `KDIR ?= /lib/modules/$(shell uname -r)/build`. Biến này chỉ đúng trên Pi.

### Quy ước tên
- Tên file module: `usb_mouse.ko`. Tên driver (`.name` trong `usb_driver`): **`usb_mouse_drv`**.
  Đường dẫn sysfs sẽ là `/sys/bus/usb/drivers/usb_mouse_drv/`. Báo cáo mẫu bị lệch giữa hai tên này, nhóm không lặp lại lỗi đó.
- Mọi symbol không `static` phải có tiền tố `usb_mouse_`.

## 4. Môi trường và quy trình làm việc

### 4.1 Mô hình: viết code trên WSL, build và test trên Pi

| Việc | Ở đâu | Ghi chú |
|---|---|---|
| Soạn code, git, viết báo cáo | **WSL** (VS Code + extension WSL) | Máy Windows của từng thành viên |
| Build module (`make`) | **Raspberry Pi** | WSL **không build được**: kernel WSL2 không có gói headers |
| Nạp module, test với chuột thật | **Raspberry Pi** | Điều khiển qua SSH từ máy Windows |

**Đẩy code sang Pi thường xuyên, sau mỗi thay đổi nhỏ, không đợi viết xong.**
Vì WSL không biên dịch được module, lỗi biên dịch chỉ lộ ra khi `make` trên Pi.
Viết nhiều rồi mới build sẽ ra một loạt lỗi khó dò.

Cách khác cũng được: dùng extension **Remote-SSH** của VS Code để mở thẳng thư mục dự án trên Pi,
sửa và build ngay ở đó. Khi làm vậy, nhớ commit/push từ Pi để không lệch với bản trên WSL.

### 4.2 Chuẩn bị Pi (một lần)
- Dùng **Raspberry Pi OS 64-bit**, bản Bookworm trở lên. Bản 32-bit trên Pi 4/5 có thể boot kernel 64-bit,
  khi đó build module bị lệch kiến trúc.
- Bật SSH (trong Raspberry Pi Imager hoặc `sudo raspi-config` → Interface Options → SSH).
- Cài đặt (gom trong `scripts/setup_pi.sh`):
  ```bash
  sudo apt update && sudo apt full-upgrade && sudo reboot   # reboot để uname -r khớp headers
  sudo apt install build-essential git rsync evtest usbutils linux-headers-$(uname -r)
  # Bản OS cũ hơn Bookworm: dùng raspberrypi-kernel-headers thay cho linux-headers-$(uname -r)
  ls /lib/modules/$(uname -r)/build      # phải tồn tại
  ```
- Model Pi, phiên bản OS và `uname -r` của nhóm: _(điền sau khi có máy)_.

### 4.3 Đồng bộ code WSL → Pi
- **Chính thức:** dùng git với một remote chung (GitHub/GitLab). Trên WSL `git push`, trên Pi `git pull`.
- **Build thử nhanh** (chưa muốn commit): `PI_HOST=<user>@<ip-của-pi> scripts/sync_to_pi.sh`.
  Script rsync code (bỏ các file trong `.gitignore`) sang `~/usb_mouse_driver` trên Pi rồi chạy `make` qua SSH.
  - Dùng **IP** của Pi (chạy `hostname -I` trên Pi). WSL2 thường không phân giải được tên `raspberrypi.local`.
  - Raspberry Pi OS mới không còn user mặc định `pi`. Dùng user đã tạo lúc cài OS.
  - Cài SSH key (`ssh-copy-id <user>@<ip>`) để khỏi nhập mật khẩu mỗi lần.

### 4.4 Build, nạp, test trên Pi
```bash
make                                  # build ra usb_mouse.ko
lsusb                                 # thấy chuột, ghi lại VID:PID
ls /sys/bus/usb/drivers/usbhid/       # tìm interface của chuột, ví dụ 1-1.3:1.0
make load                             # insmod + unbind usbhid + bind usb_mouse_drv
ls /sys/bus/usb/drivers/usb_mouse_drv/   # interface phải nằm ở đây
sudo evtest                           # chọn device do driver nhóm tạo ra
make log                              # dmesg -w lọc theo usb_mouse
make unload                           # rmmod, trả interface về usbhid
```
- **Không gõ `sudo make`.** Makefile tự gọi `sudo` cho script. Nếu chạy cả `make` dưới quyền root,
  file build sẽ thuộc root và lần build sau bằng user thường bị lỗi quyền.
- Trên kernel của Pi, `usbhid` thường **build sẵn vào kernel**, không `rmmod` được.
  Phải unbind interface qua sysfs:
  `echo -n "1-1.3:1.0" | sudo tee /sys/bus/usb/drivers/usbhid/unbind`.
  Kiểm tra: nếu `/sys/module/usbhid/initstate` không tồn tại thì `usbhid` là built-in.
- **Luôn thao tác qua SSH.** Khi unbind chuột hoặc driver lỗi, Pi mất chuột. Nếu kernel oops thì phải reboot Pi.

### 4.5 Có cần mua màn hình rời cho Pi không? — **Không bắt buộc**

| Mục đích | Cách làm | Cần màn hình? |
|---|---|---|
| Kiểm tra driver đúng (khi phát triển) | SSH vào Pi, xem `evtest` và `dmesg`: thấy `EV_REL`/`EV_KEY` khi di và bấm chuột | Không |
| Xem con trỏ di chuyển trên desktop | **VNC**: bật trong `raspi-config` → Interface Options → VNC, rồi xem desktop Pi bằng VNC Viewer trên Windows. Pi chạy không màn hình thì đặt độ phân giải ở Display Options → VNC Resolution | Không |
| Demo khi bảo vệ đồ án | Cắm Pi vào TV hoặc màn hình HDMI có sẵn (mượn là đủ). Pi 4/5 cần cáp hoặc đầu chuyển **micro-HDMI** | Mượn được thì tốt nhất |

> Khi demo qua VNC: **đừng di chuột của laptop trong cửa sổ VNC**. VNC cũng điều khiển được con trỏ Pi,
> nên người xem sẽ không phân biệt được con trỏ do chuột USB (qua driver nhóm) hay do VNC di chuyển.
> Bằng chứng chắc chắn nhất là `evtest` cho thấy sự kiện đến từ input device do `usb_mouse_drv` tạo ra.

## 5. Quy ước code

- Theo **Linux kernel coding style**: tab 8 cột, dòng ≤ 100 ký tự, `/* */` cho comment nhiều dòng.
  Trước khi commit chạy `checkpatch.pl --no-tree -f src/*.c`. Script này lấy từ thư mục `scripts/` của source kernel
  (gói headers của Pi có thể không kèm).
- Build không được có warning: `make W=1`.
- Log dùng `dev_info/dev_err/dev_dbg(&intf->dev, ...)` thay cho `printk` trần.
  Đặt `#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt` ở đầu mỗi file `.c`.
- **Không log trong callback URB** ở chế độ thường. Hàm này chạy trong ngữ cảnh ngắt và được gọi mỗi 1–10 ms.
  Chỉ dùng `dev_dbg` (bật bằng dynamic debug) hoặc `printk_ratelimited`.
- Trong callback URB chỉ dùng `GFP_ATOMIC`. Trong `probe()` dùng `GFP_KERNEL`.
- `MODULE_LICENSE("GPL")`, `MODULE_AUTHOR`, `MODULE_DESCRIPTION`, `MODULE_DEVICE_TABLE(usb, ...)`.
- Comment trong code có thể viết tiếng Việt, nhưng tên biến và hàm viết tiếng Anh.

## 6. Lỗi trong báo cáo mẫu cần tránh

Báo cáo tham khảo (`docs/reference/`) có code **không build được hoặc sai**. Nhóm **không copy nguyên văn**:

| Lỗi trong báo cáo mẫu | Cách đúng |
|---|---|
| `usb_alloc_conherent` / `usb_free_conherent` (sai chính tả) | `usb_alloc_coherent` / `usb_free_coherent` |
| Không định nghĩa `usb_mouse_id_table`, `usb_mouse_open/close` | Định nghĩa đủ, kèm `MODULE_DEVICE_TABLE(usb, ...)` |
| Dùng hàm trước khi khai báo (`usb_driver` đặt trên `probe`) | Khai báo prototype trong `usb_mouse.h` |
| Không set capability cho `input_dev` (`evbit/keybit/relbit`) | `input_set_capability()` cho từng nút và trục |
| Không set `name/phys/id` cho `input_dev` | `usb_make_path`, `usb_to_input_id`, `dev.parent = &intf->dev` |
| `}` thừa sau `pr_info("Side button 2...")` | |
| `pr_info` mỗi gói tin trong IRQ handler | Bỏ, hoặc dùng `dev_dbg` |
| `disconnect` gọi `input_free_device` sau `input_unregister_device` (double free) | Sau khi unregister thì **không** free input_dev. Thêm `usb_kill_urb` trước `usb_free_urb` |
| Cấp buffer bằng `GFP_ATOMIC` trong `probe` | `GFP_KERNEL` |
| `.name = "usb_mouse_basic"` nhưng `ls .../usb_mouse_driver/` | Dùng một tên thống nhất (`usb_mouse_drv`) |
| `echo =n` | `echo -n` |

Tham khảo code chuẩn: `drivers/hid/usbhid/usbmouse.c` của **đúng phiên bản kernel trên Pi**
(xem trên https://elixir.bootlin.com hoặc repo `raspberrypi/linux`).

## 7. Định dạng báo cáo boot protocol của chuột

| Byte | Ý nghĩa |
|---|---|
| 0 | Bit 0: trái, bit 1: phải, bit 2: giữa, bit 3: side, bit 4: extra |
| 1 | ΔX (signed 8-bit) |
| 2 | ΔY (signed 8-bit, dương là đi xuống) |
| 3 | Wheel (signed 8-bit). **Không bắt buộc** trong boot protocol, nhiều chuột vẫn gửi |

Buffer nhận tối đa 8 byte. Phải kiểm tra `urb->actual_length` trước khi đọc byte 3.

## 8. Quy trình Git của nhóm

Remote: https://github.com/VuongVanNguyen/Driver-for-USB-Mouse

**Quy tắc:** không ai commit thẳng vào `main`. Mỗi việc làm trên một nhánh riêng, merge qua Pull Request (PR).
`main` lúc nào cũng phải build được trên Pi.

### Mỗi lần bắt đầu một việc
```bash
git switch main
git pull                                  # lấy code mới nhất của cả nhóm
git switch -c feature/probe               # tạo nhánh, đặt tên theo việc
```
Tên nhánh: `feature/<việc>` (probe, input, irq, sysfs...), `fix/<lỗi>`, `docs/<nội dung>`.

### Trong lúc làm
```bash
PI_HOST=<user>@<ip> scripts/sync_to_pi.sh # build thử trên Pi thường xuyên
git status                                # xem file nào đã đổi
git diff                                  # xem chi tiết thay đổi
git add src/usb_mouse_probe.c             # chọn file cần commit (tránh "git add ." bừa)
git commit -m "probe: cấp phát URB và buffer DMA"
```
- Commit nhỏ, mỗi commit một ý. Message viết theo dạng `<phần>: <làm gì>`, ví dụ `irq: xử lý byte wheel`.
- Chỉ commit khi code **build được trên Pi** (hoặc ghi rõ `WIP` nếu cần lưu tạm).

### Khi xong việc
```bash
git push -u origin feature/probe          # lần đầu đẩy nhánh; các lần sau chỉ cần git push
```
1. Lên GitHub, bấm **Compare & pull request** để tạo PR vào `main`.
   Ghi vào PR: làm gì, đã test trên Pi thế nào (kèm log `dmesg` hoặc `evtest` nếu có).
2. Ít nhất **một thành viên khác** đọc và approve.
3. Bấm **Merge**, rồi xóa nhánh trên GitHub.
4. Về máy: `git switch main && git pull`, xóa nhánh cũ bằng `git branch -d feature/probe`.

### Khi `main` đã có code mới trong lúc mình đang làm
```bash
git switch main && git pull
git switch feature/probe
git merge main                            # có conflict thì sửa file, git add, git commit
```
`src/usb_mouse.h` là file dùng chung. Muốn đổi struct hoặc prototype thì báo cả nhóm trước,
và tách thành PR riêng, nhỏ, merge sớm để người khác cập nhật theo.

### Không commit
File build (`*.o`, `*.ko`...) đã có trong `.gitignore`. Không commit mật khẩu, token, IP của Pi hay file báo cáo nháp quá lớn.

## 9. Ghi chú cho Claude Code
- Claude chạy trên **WSL**, nơi không có headers khớp kernel. **Không chạy `make` module trên WSL.**
  Việc build và test diễn ra trên Pi; trên WSL chỉ soạn code và kiểm tra bằng mắt hoặc review.
- Không tự chạy `insmod`, `rmmod`, `ssh` sang Pi hay ghi vào `/sys/bus/usb/...` khi chưa được yêu cầu.
- **Không thêm dòng `Co-Authored-By: Claude ...`** (hay bất kỳ dòng ghi công Claude nào) vào commit message
  hoặc mô tả PR. Nhóm không muốn Claude xuất hiện trong danh sách contributors.
- Khi viết hoặc sửa code, đối chiếu với `drivers/hid/usbhid/usbmouse.c` cùng phiên bản kernel đang chạy trên Pi.
  Chú ý API thay đổi theo phiên bản, ví dụ `usb_maxpacket()` chỉ còn 2 tham số từ 5.19.
