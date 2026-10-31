// SPDX-License-Identifier: GPL-2.0
/*
 * usb_mouse_main.c - Khởi tạo module và đăng ký USB driver với usbcore.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/usb.h>
#include <linux/hid.h>

#include "usb_mouse.h"

/* Nhận mọi interface HID dạng boot mouse, giống drivers/hid/usbhid/usbmouse.c */
static const struct usb_device_id usb_mouse_id_table[] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
			     USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }
};
MODULE_DEVICE_TABLE(usb, usb_mouse_id_table);

static struct usb_driver usb_mouse_driver = {
	.name		= USB_MOUSE_DRV_NAME,
	.probe		= usb_mouse_probe,
	.disconnect	= usb_mouse_disconnect,
	.id_table	= usb_mouse_id_table,
};

/* TODO (mở rộng): module_param swap_buttons, invert_y, sensitivity */

static int __init usb_mouse_init(void)
{
	int ret;

	ret = usb_register(&usb_mouse_driver);
	if (ret) {
		pr_err("usb_register failed: %d\n", ret);
		return ret;
	}

	pr_info("driver registered\n");
	return 0;
}

static void __exit usb_mouse_exit(void)
{
	usb_deregister(&usb_mouse_driver);
	pr_info("driver deregistered\n");
}

module_init(usb_mouse_init);
module_exit(usb_mouse_exit);

MODULE_AUTHOR("TODO: tên nhóm");
MODULE_DESCRIPTION("USB boot-protocol mouse driver (HUST OS project)");
MODULE_LICENSE("GPL");
