// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/usb.h>

#include "usb_mouse.h"

static ssize_t packet_count_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev);
    struct usb_mouse *mouse = usb_get_intfdata(intf);

    if (!mouse)
        return -ENODEV;

    return sysfs_emit(buf, "%d\n",
                      atomic_read(&mouse->packet_count));
}

static ssize_t error_count_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev);
    struct usb_mouse *mouse = usb_get_intfdata(intf);

    if (!mouse)
        return -ENODEV;

    return sysfs_emit(buf, "%d\n",
                      atomic_read(&mouse->error_count));
}

static ssize_t submit_count_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev);
    struct usb_mouse *mouse = usb_get_intfdata(intf);

    if (!mouse)
        return -ENODEV;

    return sysfs_emit(buf, "%d\n",
                      atomic_read(&mouse->submit_count));
}

static DEVICE_ATTR_RO(packet_count);
static DEVICE_ATTR_RO(error_count);
static DEVICE_ATTR_RO(submit_count);

static struct attribute *usb_mouse_attrs[] = {
    &dev_attr_packet_count.attr,
    &dev_attr_error_count.attr,
    &dev_attr_submit_count.attr,
    NULL
};

static const struct attribute_group usb_mouse_attr_group = {
    .attrs = usb_mouse_attrs,
};

int usb_mouse_sysfs_create(struct usb_interface *intf)
{
    return sysfs_create_group(&intf->dev.kobj,
                              &usb_mouse_attr_group);
}

void usb_mouse_sysfs_remove(struct usb_interface *intf)
{
    sysfs_remove_group(&intf->dev.kobj,
                       &usb_mouse_attr_group);
}
