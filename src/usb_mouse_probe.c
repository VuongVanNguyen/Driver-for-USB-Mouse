// SPDX-License-Identifier: GPL-2.0
/*
 * usb_mouse_probe.c - probe()/disconnect(): cấp phát và giải phóng tài nguyên cho mỗi chuột.
 *
 * Đối chiếu: drivers/hid/usbhid/usbmouse.c (usb_mouse_probe/usb_mouse_disconnect).
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/minmax.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usb/input.h>	/* usb_to_input_id() */

#include "usb_mouse.h"

/*
 * Điền mouse->name và mouse->phys. usb_mouse_input_init() chỉ gán con trỏ
 * input->name/phys trỏ vào hai mảng này, nên phải điền trước khi register.
 */
static void usb_mouse_fill_names(struct usb_mouse *mouse)
{
	struct usb_device *udev = mouse->usbdev;

	snprintf(mouse->name, sizeof(mouse->name), "%s%s%s",
		 udev->manufacturer ?: "",
		 udev->manufacturer && udev->product ? " " : "",
		 udev->product ?: "");
	if (!mouse->name[0])
		snprintf(mouse->name, sizeof(mouse->name), "USB Mouse %04x:%04x",
			 le16_to_cpu(udev->descriptor.idVendor),
			 le16_to_cpu(udev->descriptor.idProduct));
	/* Hậu tố để evtest/dmesg phân biệt được device do driver nhóm tạo ra */
	strlcat(mouse->name, " (" USB_MOUSE_DRV_NAME ")", sizeof(mouse->name));

	usb_make_path(udev, mouse->phys, sizeof(mouse->phys));
	strlcat(mouse->phys, "/input0", sizeof(mouse->phys));
}

int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_mouse *mouse;
	unsigned int pipe;
	int maxp, ret;

	/* 1. Boot mouse chỉ có đúng một endpoint: interrupt IN */
	if (interface->desc.bNumEndpoints != 1) {
		dev_err(&intf->dev, "expected 1 endpoint, got %u\n",
			interface->desc.bNumEndpoints);
		return -ENODEV;
	}

	endpoint = &interface->endpoint[0].desc;
	if (!usb_endpoint_is_int_in(endpoint)) {
		dev_err(&intf->dev, "endpoint is not interrupt IN\n");
		return -ENODEV;
	}

	pipe = usb_rcvintpipe(udev, endpoint->bEndpointAddress);
	/* usb_endpoint_maxp() thay usb_maxpacket(): chữ ký không đổi theo bản kernel */
	maxp = min_t(int, usb_endpoint_maxp(endpoint), USB_MOUSE_BUF_LEN);

	/* 2. Trạng thái của chuột này, chép cấu hình mặc định từ module_param */
	mouse = kzalloc(sizeof(*mouse), GFP_KERNEL);
	if (!mouse)
		return -ENOMEM;

	mouse->usbdev = udev;
	mouse->intf = intf;
	mouse->swap_buttons = usb_mouse_swap_buttons;
	mouse->invert_y = usb_mouse_invert_y;
	mouse->sensitivity = clamp(usb_mouse_sensitivity,
				   USB_MOUSE_SENS_MIN, USB_MOUSE_SENS_MAX);

	/* 3. Buffer DMA nhận báo cáo và URB interrupt */
	mouse->data = usb_alloc_coherent(udev, USB_MOUSE_BUF_LEN, GFP_KERNEL,
					 &mouse->data_dma);
	if (!mouse->data) {
		ret = -ENOMEM;
		goto err_free_mouse;
	}

	mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!mouse->irq) {
		ret = -ENOMEM;
		goto err_free_buf;
	}

	/* 4. input_dev: cần mouse->intf và mouse->name/phys đã có */
	usb_mouse_fill_names(mouse);

	ret = usb_mouse_input_init(mouse);
	if (ret)
		goto err_free_urb;

	/* usb_mouse_input_init() mới set bustype, bổ sung vendor/product/version */
	usb_to_input_id(udev, &mouse->input->id);

	/*
	 * 5. Điền URB trước khi register: register có thể gọi open() ngay
	 * (vd /dev/input/mice đang mở) và open() submit URB này.
	 */
	usb_fill_int_urb(mouse->irq, udev, pipe, mouse->data, maxp,
			 usb_mouse_irq, mouse, endpoint->bInterval);
	mouse->irq->transfer_dma = mouse->data_dma;
	mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	/* 6. Đăng ký với input core */
	ret = input_register_device(mouse->input);
	if (ret) {
		dev_err(&intf->dev, "input_register_device failed: %d\n", ret);
		goto err_free_input;
	}

	usb_set_intfdata(intf, mouse);
	dev_info(&intf->dev, "%s on %s\n", mouse->name, mouse->phys);
	return 0;

err_free_input:
	/* Chưa register thành công nên free được (khác với disconnect) */
	input_free_device(mouse->input);
err_free_urb:
	usb_free_urb(mouse->irq);
err_free_buf:
	usb_free_coherent(udev, USB_MOUSE_BUF_LEN, mouse->data, mouse->data_dma);
err_free_mouse:
	kfree(mouse);
	return ret;
}

void usb_mouse_disconnect(struct usb_interface *intf)
{
	struct usb_mouse *mouse = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	if (!mouse)
		return;

	/* Dừng URB trước, để callback không còn chạy khi giải phóng bên dưới */
	usb_kill_urb(mouse->irq);
	/* Sau unregister, input core tự free input_dev: KHÔNG gọi input_free_device */
	input_unregister_device(mouse->input);
	usb_free_urb(mouse->irq);
	usb_free_coherent(mouse->usbdev, USB_MOUSE_BUF_LEN, mouse->data, mouse->data_dma);
	kfree(mouse);

	dev_info(&intf->dev, "disconnected\n");
}
