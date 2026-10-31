// SPDX-License-Identifier: GPL-2.0
/*
 * usb_mouse_input.c - Tạo và cấu hình input_dev, xử lý open()/close() từ userspace.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/input.h>
#include <linux/usb.h>
#include <linux/usb/input.h>

#include "usb_mouse.h"

/*
 * TODO: static int usb_mouse_open(struct input_dev *dev)
 *   Lấy mouse bằng input_get_drvdata(), usb_submit_urb(mouse->irq, GFP_KERNEL).
 *
 * TODO: static void usb_mouse_close(struct input_dev *dev)
 *   usb_kill_urb(mouse->irq).
 */

/* Cấp phát input_dev cho mouse->input; chưa đăng ký (probe sẽ gọi input_register_device) */
int usb_mouse_input_init(struct usb_mouse *mouse)
{
	/*
	 * TODO:
	 * 1. input_allocate_device().
	 * 2. Tên: manufacturer + product của usbdev; phys: usb_make_path() + "/input0".
	 * 3. usb_to_input_id(), dev.parent = &mouse->intf->dev.
	 * 4. input_set_capability() cho BTN_LEFT/RIGHT/MIDDLE/SIDE/EXTRA
	 *    và REL_X/REL_Y/REL_WHEEL.
	 * 5. input_set_drvdata(input, mouse); gán open/close.
	 */
	return -ENODEV;
}
