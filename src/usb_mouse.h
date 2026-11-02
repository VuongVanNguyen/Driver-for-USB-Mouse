/* SPDX-License-Identifier: GPL-2.0 */
/*
 * usb_mouse.h - Cấu trúc dữ liệu và prototype dùng chung giữa các file của driver.
 *
 * Đây là "hợp đồng" giữa các module: đổi gì ở đây phải báo cả nhóm.
 */
#ifndef _USB_MOUSE_H
#define _USB_MOUSE_H

#include <linux/types.h>
#include <linux/usb.h>
#include <linux/input.h>

#define USB_MOUSE_DRV_NAME	"usb_mouse_drv"
#define USB_MOUSE_BUF_LEN	8	/* boot protocol: 3-4 byte, cấp dư 8 byte */

/* Bit trạng thái nút trong byte 0 của báo cáo boot protocol */
#define USB_MOUSE_BTN_LEFT	0x01
#define USB_MOUSE_BTN_RIGHT	0x02
#define USB_MOUSE_BTN_MIDDLE	0x04
#define USB_MOUSE_BTN_SIDE	0x08
#define USB_MOUSE_BTN_EXTRA	0x10

/* Trạng thái của một con chuột đang được driver quản lý (mỗi interface một struct) */
struct usb_mouse {
	char name[128];			/* tên hiển thị của input device */
	char phys[64];			/* đường dẫn vật lý, vd usb-0000:01:00.0-1.3/input0 */
	struct usb_device *usbdev;
	struct usb_interface *intf;
	struct input_dev *input;
	struct urb *irq;		/* URB interrupt IN, submit lại sau mỗi lần nhận */
	signed char *data;		/* buffer DMA nhận báo cáo, USB_MOUSE_BUF_LEN byte */
	dma_addr_t data_dma;

	/* Cấu hình, đổi lúc chạy qua sysfs. irq đọc bằng READ_ONCE() */
	bool swap_buttons;		/* đổi nút trái/phải */
	bool invert_y;			/* đảo trục Y */
	int sensitivity;		/* phần trăm, 100 = giữ nguyên, 0 = chưa khởi tạo */

	/* Thống kê và trạng thái, chỉ callback irq ghi */
	unsigned long packets;		/* số gói hợp lệ */
	unsigned long clicks;		/* số lần nhấn nút (cạnh lên) */
	unsigned long errors;		/* số gói URB lỗi */
	u8 last_buttons;		/* trạng thái nút của gói trước, để đếm click */
	int rem_x, rem_y;		/* phần dư khi nhân độ nhạy, tránh mất chuyển động nhỏ */
};

#define USB_MOUSE_SENS_MIN	10
#define USB_MOUSE_SENS_MAX	500
#define USB_MOUSE_SENS_DEFAULT	100

/* usb_mouse_main.c: giá trị mặc định đặt lúc nạp module (module_param);
 * probe() chép vào từng struct usb_mouse khi cắm chuột.
 */
extern bool usb_mouse_swap_buttons;
extern bool usb_mouse_invert_y;
extern int usb_mouse_sensitivity;

/* usb_mouse_sysfs.c: truyền vào .dev_groups của usb_driver */
extern const struct attribute_group *usb_mouse_groups[];

/* usb_mouse_probe.c */
int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id);
void usb_mouse_disconnect(struct usb_interface *intf);

/* usb_mouse_input.c */
int usb_mouse_input_init(struct usb_mouse *mouse);

/* usb_mouse_irq.c */
void usb_mouse_irq(struct urb *urb);

#endif /* _USB_MOUSE_H */
