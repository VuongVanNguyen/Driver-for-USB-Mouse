#!/usr/bin/env bash
# Hàm dùng chung cho load.sh / unload.sh. Không chạy trực tiếp.

DRV=usb_mouse_drv
MOD=usb_mouse

require_root() {
	if [[ $EUID -ne 0 ]]; then
		echo "Cần chạy với sudo" >&2
		exit 1
	fi
}

# In tên các interface HID boot mouse (class 03, subclass 01, protocol 02), vd 1-1.3:1.0
find_mouse_intfs() {
	local d
	for d in /sys/bus/usb/devices/*:*; do
		if [[ "$(cat "$d/bInterfaceClass" 2>/dev/null)" == 03 &&
		      "$(cat "$d/bInterfaceSubClass" 2>/dev/null)" == 01 &&
		      "$(cat "$d/bInterfaceProtocol" 2>/dev/null)" == 02 ]]; then
			basename "$d"
		fi
	done
}

# In tên driver đang gắn với interface, rỗng nếu chưa gắn
current_driver() {
	local link="/sys/bus/usb/devices/$1/driver"
	if [[ -L $link ]]; then
		basename "$(readlink "$link")"
	fi
}

module_loaded() {
	[[ -d /sys/module/$MOD ]]
}
