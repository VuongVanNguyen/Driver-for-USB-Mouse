# USB Mouse Driver — Bài tập lớn Hệ điều hành

Kernel module Linux điều khiển chuột USB (HID boot protocol), test trên Raspberry Pi.
Chi tiết đề tài, cấu trúc và quy ước: xem [CLAUDE.md](CLAUDE.md).

## Bắt đầu nhanh (trên Pi)

```bash
./scripts/setup_pi.sh     # một lần: toolchain + kernel headers
make                      # build usb_mouse.ko
make load                 # nạp module, chuyển chuột từ usbhid sang usb_mouse_drv
sudo evtest               # xem sự kiện
make unload               # gỡ module, trả chuột về usbhid
```

## Đồng bộ từ WSL sang Pi

```bash
PI_HOST=<user>@<ip-của-pi> scripts/sync_to_pi.sh
```
