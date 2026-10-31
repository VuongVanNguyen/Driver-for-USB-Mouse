#!/usr/bin/env bash
# Chạy trên Pi: gỡ usb_mouse.ko và trả mọi interface chuột chưa có driver về usbhid.
# Dùng: sudo scripts/unload.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/scripts/common.sh"
require_root

if module_loaded; then
	echo "rmmod $MOD"
	rmmod "$MOD"	# tự gọi disconnect() cho các interface đang gắn
fi

while read -r intf; do
	if [[ -z "$(current_driver "$intf")" ]]; then
		echo "bind $intf vào usbhid"
		echo -n "$intf" > /sys/bus/usb/drivers/usbhid/bind || echo "bind $intf thất bại" >&2
	fi
done < <(find_mouse_intfs)
