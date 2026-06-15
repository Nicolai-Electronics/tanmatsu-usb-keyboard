#pragma once

typedef enum {
    USB_DEBUG    = 0,
    USB_DEVICE   = 1,
    USB_DISABLED = 2,
} usb_mode_t;

void usb_mode_set(usb_mode_t mode);
