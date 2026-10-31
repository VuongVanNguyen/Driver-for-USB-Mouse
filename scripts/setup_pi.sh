#!/usr/bin/env bash
# Chạy trên Pi (một lần): cài toolchain, kernel headers và công cụ test.
# Nên chạy "sudo apt full-upgrade && sudo reboot" trước để uname -r khớp với headers.
set -euo pipefail

sudo apt update
sudo apt install -y build-essential git rsync evtest usbutils

if ! sudo apt install -y "linux-headers-$(uname -r)"; then
	echo "Không có linux-headers-$(uname -r), thử raspberrypi-kernel-headers (OS cũ)" >&2
	sudo apt install -y raspberrypi-kernel-headers
fi

if [[ -d /lib/modules/$(uname -r)/build ]]; then
	echo "OK: headers cho $(uname -r) đã sẵn sàng"
else
	echo "Thiếu /lib/modules/$(uname -r)/build: hãy full-upgrade, reboot rồi chạy lại" >&2
	exit 1
fi
