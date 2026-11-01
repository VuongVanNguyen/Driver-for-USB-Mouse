// SPDX-License-Identifier: GPL-2.0
/*
 * usb_mouse_sysfs.c - Thuộc tính sysfs: cấu hình lúc chạy và thống kê.
 *
 * File nằm ở /sys/bus/usb/drivers/usb_mouse_drv/<interface>/. Mảng usb_mouse_groups
 * được gán vào .dev_groups của usb_driver, nên driver core tự tạo file sau khi probe
 * thành công và tự xóa trước disconnect, không cần gọi sysfs_create_group().
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/usb.h>

#include "usb_mouse.h"

static struct usb_mouse *to_mouse(struct device *dev)
{
	return usb_get_intfdata(to_usb_interface(dev));
}

/* ---- cấu hình (đọc/ghi) ---- */

static ssize_t swap_buttons_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_mouse *mouse = to_mouse(dev);

	if (!mouse)
		return -ENODEV;
	return sysfs_emit(buf, "%d\n", READ_ONCE(mouse->swap_buttons));
}

static ssize_t swap_buttons_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct usb_mouse *mouse = to_mouse(dev);
	bool val;
	int ret;

	if (!mouse)
		return -ENODEV;
	ret = kstrtobool(buf, &val);
	if (ret)
		return ret;
	WRITE_ONCE(mouse->swap_buttons, val);
	return count;
}
static DEVICE_ATTR_RW(swap_buttons);

static ssize_t invert_y_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_mouse *mouse = to_mouse(dev);

	if (!mouse)
		return -ENODEV;
	return sysfs_emit(buf, "%d\n", READ_ONCE(mouse->invert_y));
}

static ssize_t invert_y_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct usb_mouse *mouse = to_mouse(dev);
	bool val;
	int ret;

	if (!mouse)
		return -ENODEV;
	ret = kstrtobool(buf, &val);
	if (ret)
		return ret;
	WRITE_ONCE(mouse->invert_y, val);
	return count;
}
static DEVICE_ATTR_RW(invert_y);

static ssize_t sensitivity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_mouse *mouse = to_mouse(dev);
	int sens;

	if (!mouse)
		return -ENODEV;
	sens = READ_ONCE(mouse->sensitivity);
	return sysfs_emit(buf, "%d\n", sens > 0 ? sens : USB_MOUSE_SENS_DEFAULT);
}

static ssize_t sensitivity_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct usb_mouse *mouse = to_mouse(dev);
	int val, ret;

	if (!mouse)
		return -ENODEV;
	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return ret;
	if (val < USB_MOUSE_SENS_MIN || val > USB_MOUSE_SENS_MAX)
		return -EINVAL;
	WRITE_ONCE(mouse->sensitivity, val);
	return count;
}
static DEVICE_ATTR_RW(sensitivity);

/* ---- thống kê (chỉ đọc) ---- */

static ssize_t packets_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_mouse *mouse = to_mouse(dev);

	if (!mouse)
		return -ENODEV;
	return sysfs_emit(buf, "%lu\n", READ_ONCE(mouse->packets));
}
static DEVICE_ATTR_RO(packets);

static ssize_t clicks_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_mouse *mouse = to_mouse(dev);

	if (!mouse)
		return -ENODEV;
	return sysfs_emit(buf, "%lu\n", READ_ONCE(mouse->clicks));
}
static DEVICE_ATTR_RO(clicks);

static ssize_t errors_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_mouse *mouse = to_mouse(dev);

	if (!mouse)
		return -ENODEV;
	return sysfs_emit(buf, "%lu\n", READ_ONCE(mouse->errors));
}
static DEVICE_ATTR_RO(errors);

/* ---- đặt lại thống kê (chỉ ghi): ghi gì cũng được ---- */

static ssize_t reset_stats_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct usb_mouse *mouse = to_mouse(dev);

	if (!mouse)
		return -ENODEV;
	WRITE_ONCE(mouse->packets, 0);
	WRITE_ONCE(mouse->clicks, 0);
	WRITE_ONCE(mouse->errors, 0);
	return count;
}
static DEVICE_ATTR_WO(reset_stats);

static struct attribute *usb_mouse_attrs[] = {
	&dev_attr_swap_buttons.attr,
	&dev_attr_invert_y.attr,
	&dev_attr_sensitivity.attr,
	&dev_attr_packets.attr,
	&dev_attr_clicks.attr,
	&dev_attr_errors.attr,
	&dev_attr_reset_stats.attr,
	NULL,
};

static const struct attribute_group usb_mouse_group = {
	.attrs = usb_mouse_attrs,
};

/* Không dùng ATTRIBUTE_GROUPS(): macro đó sinh mảng static, usb_mouse_main.c không thấy */
const struct attribute_group *usb_mouse_groups[] = {
	&usb_mouse_group,
	NULL,
};
