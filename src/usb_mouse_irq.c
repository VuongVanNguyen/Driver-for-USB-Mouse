// SPDX-License-Identifier: GPL-2.0
/*
 * usb_mouse_irq.c - Callback hoàn thành URB: phân tích báo cáo chuột, đẩy sự kiện vào input core.
 *
 * Chạy trong ngữ cảnh ngắt: không ngủ, chỉ GFP_ATOMIC, không log mỗi gói tin.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/bitops.h>	/* hweight8() */
#include <linux/input.h>
#include <linux/usb.h>

#include "usb_mouse.h"

/*
 * Nhân delta với độ nhạy (phần trăm). Phần dư được cộng dồn sang gói sau,
 * nếu không thì chuyển động nhỏ (vd 1 * 50 / 100) luôn bị làm tròn về 0.
 */
static int usb_mouse_scale(int delta, int sens, int *rem)
{
	int total = delta * sens + *rem;
	int out = total / 100;

	*rem = total - out * 100;
	return out;
}

void usb_mouse_irq(struct urb *urb)
{
	struct usb_mouse *mouse = urb->context;
	signed char *data = mouse->data;
	struct input_dev *dev = mouse->input;
	bool swap, invert;
	int sens, dx, dy, ret;
	u8 btn, left, right;

	switch (urb->status) {
	case 0:
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* URB bị hủy hoặc chuột đã rút: không submit lại */
		return;
	default:
		/* Lỗi tạm thời trên đường truyền: bỏ gói này, vẫn nộp lại URB */
		mouse->errors++;
		goto resubmit;
	}

	/* Báo cáo boot protocol tối thiểu 3 byte: nút, dx, dy */
	if (urb->actual_length < 3) {
		mouse->errors++;
		goto resubmit;
	}

	swap = READ_ONCE(mouse->swap_buttons);
	invert = READ_ONCE(mouse->invert_y);
	sens = READ_ONCE(mouse->sensitivity);
	if (sens <= 0)
		sens = USB_MOUSE_SENS_DEFAULT;	/* probe chưa khởi tạo cấu hình */

	btn = (u8)data[0];
	left = btn & USB_MOUSE_BTN_LEFT;
	right = btn & USB_MOUSE_BTN_RIGHT;
	if (swap) {
		/* Giá trị báo lên input là "có nhấn hay không", đổi chỗ hai nút */
		u8 tmp = left;

		left = right;
		right = tmp;
	}

	input_report_key(dev, BTN_LEFT, !!left);
	input_report_key(dev, BTN_RIGHT, !!right);
	input_report_key(dev, BTN_MIDDLE, btn & USB_MOUSE_BTN_MIDDLE);
	input_report_key(dev, BTN_SIDE, btn & USB_MOUSE_BTN_SIDE);
	input_report_key(dev, BTN_EXTRA, btn & USB_MOUSE_BTN_EXTRA);

	dx = data[1];
	dy = invert ? -data[2] : data[2];
	if (sens != USB_MOUSE_SENS_DEFAULT) {
		dx = usb_mouse_scale(dx, sens, &mouse->rem_x);
		dy = usb_mouse_scale(dy, sens, &mouse->rem_y);
	}
	input_report_rel(dev, REL_X, dx);
	input_report_rel(dev, REL_Y, dy);

	/* Byte 3 (wheel) không bắt buộc trong boot protocol */
	if (urb->actual_length >= 4)
		input_report_rel(dev, REL_WHEEL, data[3]);

	input_sync(dev);

	/* Đếm cạnh lên: nút giữ nguyên không tính thêm click */
	mouse->clicks += hweight8(btn & ~mouse->last_buttons &
				  (USB_MOUSE_BTN_LEFT | USB_MOUSE_BTN_RIGHT |
				   USB_MOUSE_BTN_MIDDLE | USB_MOUSE_BTN_SIDE |
				   USB_MOUSE_BTN_EXTRA));
	mouse->last_buttons = btn;
	mouse->packets++;

resubmit:
	ret = usb_submit_urb(urb, GFP_ATOMIC);
	/* -EPERM: URB đang bị kill, không phải lỗi thật */
	if (ret && ret != -EPERM)
		dev_err_ratelimited(&mouse->intf->dev, "resubmit URB failed: %d\n", ret);
}
