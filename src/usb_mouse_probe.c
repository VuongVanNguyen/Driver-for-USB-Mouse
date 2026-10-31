// SPDX-License-Identifier: GPL-2.0
/*
 * usb_mouse_probe.c - probe()/disconnect(): cấp phát và giải phóng tài nguyên cho mỗi chuột.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/slab.h>
#include <linux/usb.h>

#include "usb_mouse.h"

int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	/*
	 * TODO:
	 * 1. Kiểm tra cur_altsetting có đúng 1 endpoint và là interrupt IN
	 *    (usb_endpoint_is_int_in), nếu không thì trả -ENODEV.
	 * 2. kzalloc struct usb_mouse (GFP_KERNEL), lưu usbdev và intf.
	 * 3. usb_alloc_coherent() buffer USB_MOUSE_BUF_LEN byte, usb_alloc_urb().
	 * 4. usb_mouse_input_init(mouse).
	 * 5. usb_fill_int_urb(..., usb_mouse_irq, mouse, ep->bInterval),
	 *    gán transfer_dma và cờ URB_NO_TRANSFER_DMA_MAP.
	 * 6. input_register_device(), rồi usb_set_intfdata(intf, mouse).
	 * Lỗi ở bước nào thì goto nhãn giải phóng ngược lại các bước trước.
	 */
	dev_info(&intf->dev, "probe: not implemented yet\n");
	return -ENODEV;
}

void usb_mouse_disconnect(struct usb_interface *intf)
{
	/*
	 * TODO:
	 * 1. mouse = usb_get_intfdata(intf); usb_set_intfdata(intf, NULL).
	 * 2. usb_kill_urb() trước khi giải phóng bất cứ thứ gì.
	 * 3. input_unregister_device() (KHÔNG gọi input_free_device sau đó).
	 * 4. usb_free_urb(), usb_free_coherent(), kfree(mouse).
	 */
}
