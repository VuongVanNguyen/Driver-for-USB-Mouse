# SPDX-License-Identifier: GPL-2.0
#
# Chạy trên Raspberry Pi (WSL không có kernel headers khớp kernel).
# Danh sách object của module nằm trong file Kbuild.

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(MAKE) -C tools clean

# Không chạy "sudo make ...": script tự gọi sudo, tránh tạo file build thuộc root
load: all
	sudo ./scripts/load.sh

unload:
	sudo ./scripts/unload.sh

log:
	sudo dmesg -w | grep --line-buffered usb_mouse

tools:
	$(MAKE) -C tools

.PHONY: all clean load unload log tools
