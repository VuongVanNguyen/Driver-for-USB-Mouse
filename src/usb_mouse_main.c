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
	.dev_groups	= usb_mouse_groups,	/* sysfs: usb_mouse_sysfs.c */
};

/*
 * Giá trị mặc định đặt lúc nạp module; probe() chép vào từng struct usb_mouse.
 * Đổi lúc chạy cho một con chuột cụ thể thì dùng sysfs (usb_mouse_sysfs.c).
 */
bool usb_mouse_swap_buttons;
module_param_named(swap_buttons, usb_mouse_swap_buttons, bool, 0644);
MODULE_PARM_DESC(swap_buttons, "Đổi nút trái/phải (mặc định: tắt)");

bool usb_mouse_invert_y;
module_param_named(invert_y, usb_mouse_invert_y, bool, 0644);
MODULE_PARM_DESC(invert_y, "Đảo trục Y (mặc định: tắt)");

int usb_mouse_sensitivity = USB_MOUSE_SENS_DEFAULT;
module_param_named(sensitivity, usb_mouse_sensitivity, int, 0644);
MODULE_PARM_DESC(sensitivity,
		 "Độ nhạy phần trăm, kẹp về 10..500 (mặc định 100)");

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

MODULE_AUTHOR("Vuong-Thien-Hoang");
MODULE_DESCRIPTION("USB boot-protocol mouse driver (HUST OS project)");
MODULE_LICENSE("GPL");
