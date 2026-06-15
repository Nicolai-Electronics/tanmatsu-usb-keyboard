#include "usb_mode.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "hal/usb_serial_jtag_ll.h"

static const char* TAG = "USB mode";

static usb_mode_t current_mode = USB_DEBUG;

void usb_mode_set(usb_mode_t mode) {
    const usb_serial_jtag_pull_override_vals_t override_disable_usb = {
        .dm_pd = true, .dm_pu = false, .dp_pd = true, .dp_pu = false};
    const usb_serial_jtag_pull_override_vals_t override_enable_usb = {
        .dm_pd = false, .dm_pu = false, .dp_pd = false, .dp_pu = true};

    // Drop off the bus by removing the pull-up on USB DP
    usb_serial_jtag_ll_phy_enable_pull_override(&override_disable_usb);

    // Select USB mode by swapping and un-swapping the two PHYs
    switch (mode) {
        case USB_DEVICE:
            vTaskDelay(pdMS_TO_TICKS(500));  // Wait for disconnect before switching to device
            usb_serial_jtag_ll_phy_select(1);
            break;
        case USB_DEBUG:
        case USB_DISABLED:
        default:
            usb_serial_jtag_ll_phy_select(0);
            vTaskDelay(pdMS_TO_TICKS(500));  // Wait for disconnect after switching to debug
            break;
    }

    if (mode != USB_DISABLED) {
        // Put the device back onto the bus by re-enabling the pull-up on USB DP
        usb_serial_jtag_ll_phy_enable_pull_override(&override_enable_usb);
        usb_serial_jtag_ll_phy_disable_pull_override();
    }
    current_mode = mode;
}

usb_mode_t usb_mode_get(void) {
    return current_mode;
}
