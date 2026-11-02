// SPDX-License-Identifier: GPL-2.0

#include <linux/input.h>
#include <linux/usb.h>
#include <linux/slab.h>

#include "usb_mouse.h"

/* Bat dau nhan du lieu khi thiet bi duoc mo */
static int usb_mouse_open(struct input_dev *dev)
{
	struct usb_mouse *mouse = input_get_drvdata(dev);

	if (!mouse || !mouse->irq)
		return -ENODEV;

	return usb_submit_urb(mouse->irq, GFP_KERNEL);
}

/* Dung nhan du lieu khi thiet bi dong */
static void usb_mouse_close(struct input_dev *dev)
{
	struct usb_mouse *mouse = input_get_drvdata(dev);

	if (mouse && mouse->irq)
		usb_kill_urb(mouse->irq);
}

/* Khoi tao thiet bi input cho chuot USB */
int usb_mouse_input_init(struct usb_mouse *mouse)
{
	struct input_dev *input;

	if (!mouse)
		return -EINVAL;

	input = input_allocate_device();
	if (!input)
		return -ENOMEM;

	mouse->input = input;

	input->name = mouse->name;
	input->phys = mouse->phys;
	input->dev.parent = &mouse->intf->dev;

	input->id.bustype = BUS_USB;

	input_set_drvdata(input, mouse);

	/* Khai bao cac nut bam */
	input_set_capability(input, EV_KEY, BTN_LEFT);
	input_set_capability(input, EV_KEY, BTN_RIGHT);
	input_set_capability(input, EV_KEY, BTN_MIDDLE);
	input_set_capability(input, EV_KEY, BTN_SIDE);
	input_set_capability(input, EV_KEY, BTN_EXTRA);

	/* Khai bao cac su kien di chuyen */
	input_set_capability(input, EV_REL, REL_X);
	input_set_capability(input, EV_REL, REL_Y);
	input_set_capability(input, EV_REL, REL_WHEEL);

	input->open = usb_mouse_open;
	input->close = usb_mouse_close;

	return 0;
}
