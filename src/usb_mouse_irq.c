
// SPDX-License-Identifier: GPL-2.0

#include <linux/input.h>
#include <linux/usb.h>

#include "usb_mouse.h"

void usb_mouse_irq(struct urb *urb)
{
    struct usb_mouse *mouse = urb->context;
    unsigned char *data;
    int status;

    if (!mouse || !mouse->input)
        return;

    switch (urb->status) {
    case 0:
        break;

    case -ECONNRESET:
    case -ENOENT:
    case -ESHUTDOWN:
        return;

    default:
        atomic_inc(&mouse->error_count);
        goto resubmit;
    }

    /* Kiem tra do dai USB report */
    if (urb->actual_length < 3) {
        atomic_inc(&mouse->error_count);
        goto resubmit;
    }

    data = (unsigned char *)mouse->data;

    /* Thong ke goi du lieu hop le */
    atomic_inc(&mouse->packet_count);

    /* Xu ly cac nut bam chuot */
    input_report_key(mouse->input, BTN_LEFT,
                     !!(data[0] & USB_MOUSE_BTN_LEFT));
    input_report_key(mouse->input, BTN_RIGHT,
                     !!(data[0] & USB_MOUSE_BTN_RIGHT));
    input_report_key(mouse->input, BTN_MIDDLE,
                     !!(data[0] & USB_MOUSE_BTN_MIDDLE));
    input_report_key(mouse->input, BTN_SIDE,
                     !!(data[0] & USB_MOUSE_BTN_SIDE));
    input_report_key(mouse->input, BTN_EXTRA,
                     !!(data[0] & USB_MOUSE_BTN_EXTRA));

    /* Xu ly di chuyen chuot */
    input_report_rel(mouse->input, REL_X, (signed char)data[1]);
    input_report_rel(mouse->input, REL_Y, (signed char)data[2]);

    /* Xu ly con lan neu report co tu 4 byte */
    if (urb->actual_length >= 4) {
        input_report_rel(mouse->input, REL_WHEEL,
                         (signed char)data[3]);
    }

    input_sync(mouse->input);

resubmit:
    /* Gui lai URB de tiep tuc nhan du lieu */
    status = usb_submit_urb(urb, GFP_ATOMIC);

    if (status) {
        atomic_inc(&mouse->error_count);
        dev_err(&mouse->intf->dev,
                "Failed to resubmit URB: %d\n", status);
    } else {
        atomic_inc(&mouse->submit_count);
    }
}
