// SPDX-License-Identifier: GPL-2.0
/*
 * usb_mouse_irq.c - Callback hoàn thành URB: phân tích báo cáo chuột, đẩy sự kiện vào input core.
 *
 * Chạy trong ngữ cảnh ngắt: không ngủ, chỉ GFP_ATOMIC, không log mỗi gói tin.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/input.h>
#include <linux/usb.h>

#include "usb_mouse.h"

void usb_mouse_irq(struct urb *urb)
{
	/*
	 * TODO:
	 * 1. Xét urb->status: 0 thì xử lý; -ECONNRESET/-ENOENT/-ESHUTDOWN thì return
	 *    (URB bị hủy hoặc thiết bị đã rút); lỗi khác thì bỏ qua gói, nhảy tới resubmit.
	 * 2. Byte 0 là nút (USB_MOUSE_BTN_*), byte 1/2 là ΔX/ΔY. Byte 3 là wheel,
	 *    chỉ đọc khi urb->actual_length >= 4.
	 * 3. input_report_key()/input_report_rel(), rồi input_sync().
	 * 4. resubmit: usb_submit_urb(urb, GFP_ATOMIC), báo lỗi bằng dev_err nếu thất bại.
	 */
}
