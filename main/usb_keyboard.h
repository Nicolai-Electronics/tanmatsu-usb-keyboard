#pragma once

#include <stdint.h>

void usb_keyboard_init(void);
void usb_keyboard_send_scancode(uint32_t scancode);
