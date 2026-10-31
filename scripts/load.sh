#!/usr/bin/env bash
# Chạy trên Pi: nạp usb_mouse.ko và chuyển interface chuột từ usbhid sang usb_mouse_drv.
# Dùng: sudo scripts/load.sh [INTERFACE]    (vd 1-1.3:1.0; bỏ trống để tự tìm)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/scripts/common.sh"
require_root

if [[ $# -ge 1 ]]; then
	intfs=("$1")
else
	mapfile -t intfs < <(find_mouse_intfs)
fi
if [[ ${#intfs[@]} -eq 0 ]]; then
	echo "Không tìm thấy interface chuột (HID boot mouse)" >&2
	exit 1
fi

if ! module_loaded; then
	echo "insmod $MOD.ko"
	insmod "$ROOT/$MOD.ko"
fi

for intf in "${intfs[@]}"; do
	cur="$(current_driver "$intf")"
	if [[ $cur == "$DRV" ]]; then
		echo "$intf đã gắn với $DRV"
		continue
	fi
	if [[ -n $cur ]]; then
		echo "unbind $intf khỏi $cur"
		echo -n "$intf" > "/sys/bus/usb/drivers/$cur/unbind"
	fi
	echo "bind $intf vào $DRV"
	if ! echo -n "$intf" > "/sys/bus/usb/drivers/$DRV/bind"; then
		# probe thất bại: trả chuột về driver cũ để Pi không mất chuột
		echo "bind thất bại, trả $intf về ${cur:-usbhid} (xem dmesg)" >&2
		echo -n "$intf" > "/sys/bus/usb/drivers/${cur:-usbhid}/bind" || true
		exit 1
	fi
done
