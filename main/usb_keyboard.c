#include "usb_keyboard.h"
#include <string.h>
#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

static const char TAG[] = "usb_kbd";

// Standard USB HID keyboard report descriptor
static const uint8_t hid_report_desc[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static const uint8_t hid_config_desc[] = {
    // config: 1 configuration, 1 interface, no string, total len, remote wakeup, 100 mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // HID: interface 0, string 4, boot keyboard, report desc len, EP 0x81 IN, 8 bytes, 10 ms poll
    TUD_HID_DESCRIPTOR(0, 4, true, sizeof(hid_report_desc), 0x81, 8, 10),
};

static const char *hid_string_desc[] = {
    (char[]){0x09, 0x04}, // 0: Language — English (0x0409)
    "Nicola Electronics",  // 1: Manufacturer
    "Tanmatsu Keyboard",   // 2: Product
    "0001",                // 3: Serial number
    "HID Keyboard",        // 4: HID interface
};

// PS/2 scan code set 1 make code (index) -> USB HID keycode
// 0x00 = no mapping, 0xFF = modifier (handled via modifier_bits[])
static const uint8_t ps2_to_hid[0x59] = {
    /* 0x00 */ 0x00, // (none)
    /* 0x01 */ 0x29, // ESC
    /* 0x02 */ 0x1E, // 1
    /* 0x03 */ 0x1F, // 2
    /* 0x04 */ 0x20, // 3
    /* 0x05 */ 0x21, // 4
    /* 0x06 */ 0x22, // 5
    /* 0x07 */ 0x23, // 6
    /* 0x08 */ 0x24, // 7
    /* 0x09 */ 0x25, // 8
    /* 0x0A */ 0x26, // 9
    /* 0x0B */ 0x27, // 0
    /* 0x0C */ 0x2D, // -
    /* 0x0D */ 0x2E, // =
    /* 0x0E */ 0x2A, // Backspace
    /* 0x0F */ 0x2B, // Tab
    /* 0x10 */ 0x14, // Q
    /* 0x11 */ 0x1A, // W
    /* 0x12 */ 0x08, // E
    /* 0x13 */ 0x15, // R
    /* 0x14 */ 0x17, // T
    /* 0x15 */ 0x1C, // Y
    /* 0x16 */ 0x18, // U
    /* 0x17 */ 0x0C, // I
    /* 0x18 */ 0x12, // O
    /* 0x19 */ 0x13, // P
    /* 0x1A */ 0x2F, // [
    /* 0x1B */ 0x30, // ]
    /* 0x1C */ 0x28, // Enter
    /* 0x1D */ 0xFF, // Left Ctrl  (modifier)
    /* 0x1E */ 0x04, // A
    /* 0x1F */ 0x16, // S
    /* 0x20 */ 0x07, // D
    /* 0x21 */ 0x09, // F
    /* 0x22 */ 0x0A, // G
    /* 0x23 */ 0x0B, // H
    /* 0x24 */ 0x0D, // J
    /* 0x25 */ 0x0E, // K
    /* 0x26 */ 0x0F, // L
    /* 0x27 */ 0x33, // ;
    /* 0x28 */ 0x34, // '
    /* 0x29 */ 0x35, // `
    /* 0x2A */ 0xFF, // Left Shift  (modifier)
    /* 0x2B */ 0x31, // backslash
    /* 0x2C */ 0x1D, // Z
    /* 0x2D */ 0x1B, // X
    /* 0x2E */ 0x06, // C
    /* 0x2F */ 0x19, // V
    /* 0x30 */ 0x05, // B
    /* 0x31 */ 0x11, // N
    /* 0x32 */ 0x10, // M
    /* 0x33 */ 0x36, // ,
    /* 0x34 */ 0x37, // .
    /* 0x35 */ 0x38, // /
    /* 0x36 */ 0xFF, // Right Shift  (modifier)
    /* 0x37 */ 0x55, // KP *
    /* 0x38 */ 0xFF, // Left Alt  (modifier)
    /* 0x39 */ 0x2C, // Space
    /* 0x3A */ 0x39, // Caps Lock
    /* 0x3B */ 0x3A, // F1
    /* 0x3C */ 0x3B, // F2
    /* 0x3D */ 0x3C, // F3
    /* 0x3E */ 0x3D, // F4
    /* 0x3F */ 0x3E, // F5
    /* 0x40 */ 0x3F, // F6
    /* 0x41 */ 0x40, // F7
    /* 0x42 */ 0x41, // F8
    /* 0x43 */ 0x42, // F9
    /* 0x44 */ 0x43, // F10
    /* 0x45 */ 0x53, // Num Lock
    /* 0x46 */ 0x47, // Scroll Lock
    /* 0x47 */ 0x5F, // KP 7
    /* 0x48 */ 0x60, // KP 8
    /* 0x49 */ 0x61, // KP 9
    /* 0x4A */ 0x56, // KP -
    /* 0x4B */ 0x5C, // KP 4
    /* 0x4C */ 0x5D, // KP 5
    /* 0x4D */ 0x5E, // KP 6
    /* 0x4E */ 0x57, // KP +
    /* 0x4F */ 0x59, // KP 1
    /* 0x50 */ 0x5A, // KP 2
    /* 0x51 */ 0x5B, // KP 3
    /* 0x52 */ 0x62, // KP 0
    /* 0x53 */ 0x63, // KP .
    /* 0x54 */ 0x46, // SysRq / Print Screen
    /* 0x55 */ 0x00, // Fn (no USB HID equivalent)
    /* 0x56 */ 0x00, // (undefined)
    /* 0x57 */ 0x44, // F11
    /* 0x58 */ 0x45, // F12
};

// Modifier bit for regular PS/2 make codes that are modifier keys
static const uint8_t modifier_bits[0x59] = {
    /* 0x1D */ [0x1D] = KEYBOARD_MODIFIER_LEFTCTRL,
    /* 0x2A */ [0x2A] = KEYBOARD_MODIFIER_LEFTSHIFT,
    /* 0x36 */ [0x36] = KEYBOARD_MODIFIER_RIGHTSHIFT,
    /* 0x38 */ [0x38] = KEYBOARD_MODIFIER_LEFTALT,
};

// Extended key table (PS/2 0xE0 prefix codes)
typedef struct {
    uint16_t ps2_ext;
    uint8_t  hid_key;    // 0x00 = ignore, 0xFF = modifier
    uint8_t  modifier;   // used when hid_key == 0xFF
} ext_key_t;

static const ext_key_t ext_keys[] = {
    {0xe01c, 0x58, 0},                               // KP Enter
    {0xe01d, 0xFF, KEYBOARD_MODIFIER_RIGHTCTRL},     // Right Ctrl
    {0xe035, 0x54, 0},                               // KP /
    {0xe037, 0x46, 0},                               // Print Screen
    {0xe038, 0xFF, KEYBOARD_MODIFIER_RIGHTALT},      // Right Alt
    {0xe046, 0x48, 0},                               // Pause/Break
    {0xe047, 0x4A, 0},                               // Home
    {0xe048, 0x52, 0},                               // Arrow Up
    {0xe049, 0x4B, 0},                               // Page Up
    {0xe04b, 0x50, 0},                               // Arrow Left
    {0xe04d, 0x4F, 0},                               // Arrow Right
    {0xe04f, 0x4D, 0},                               // End
    {0xe050, 0x51, 0},                               // Arrow Down
    {0xe051, 0x4E, 0},                               // Page Down
    {0xe052, 0x49, 0},                               // Insert
    {0xe053, 0x4C, 0},                               // Delete
    {0xe05b, 0xFF, KEYBOARD_MODIFIER_LEFTGUI},       // Left Meta/Super
    {0xe05c, 0xFF, KEYBOARD_MODIFIER_RIGHTGUI},      // Right Meta/Super
    {0xe05d, 0x65, 0},                               // Application/Menu
};

// Current keyboard state
static uint8_t cur_modifier = 0;
static uint8_t cur_keys[6]  = {0};

static void key_press(uint8_t keycode) {
    for (int i = 0; i < 6; i++) {
        if (cur_keys[i] == keycode) return;
    }
    for (int i = 0; i < 6; i++) {
        if (cur_keys[i] == 0) {
            cur_keys[i] = keycode;
            return;
        }
    }
    // 6-key rollover exceeded — drop the new key
}

static void key_release(uint8_t keycode) {
    for (int i = 0; i < 6; i++) {
        if (cur_keys[i] == keycode) {
            // Shift remaining entries down to keep array compact
            for (int j = i; j < 5; j++) cur_keys[j] = cur_keys[j + 1];
            cur_keys[5] = 0;
            return;
        }
    }
}

static void send_report(void) {
    if (!tud_mounted() || !tud_hid_ready()) return;
    tud_hid_keyboard_report(0, cur_modifier, cur_keys);
}

void usb_keyboard_send_scancode(uint32_t scancode) {
    bool     is_release = (scancode & 0x80) != 0;
    uint32_t make_code  = scancode & ~(uint32_t)0x80;
    bool     is_ext     = (make_code & 0xff00) != 0;

    if (is_ext) {
        for (size_t i = 0; i < sizeof(ext_keys) / sizeof(ext_keys[0]); i++) {
            if (ext_keys[i].ps2_ext != make_code) continue;

            if (ext_keys[i].hid_key == 0xFF) {
                if (is_release)
                    cur_modifier &= ~ext_keys[i].modifier;
                else
                    cur_modifier |= ext_keys[i].modifier;
            } else if (ext_keys[i].hid_key != 0x00) {
                if (is_release)
                    key_release(ext_keys[i].hid_key);
                else
                    key_press(ext_keys[i].hid_key);
            }
            send_report();
            return;
        }
        // Unknown extended key — ignore
        return;
    }

    if (make_code >= sizeof(ps2_to_hid)) return;

    uint8_t hid_key = ps2_to_hid[make_code];
    if (hid_key == 0x00) return;

    if (hid_key == 0xFF) {
        uint8_t mod = modifier_bits[make_code];
        if (mod) {
            if (is_release)
                cur_modifier &= ~mod;
            else
                cur_modifier |= mod;
        }
    } else {
        if (is_release)
            key_release(hid_key);
        else
            key_press(hid_key);
    }

    send_report();
}

// ---- TinyUSB HID callbacks ----

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_desc;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
    // LED state (Num Lock, Caps Lock, Scroll Lock) could be handled here
}

// ---- Initialisation ----

void usb_keyboard_init(void) {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor        = NULL,
        .string_descriptor        = hid_string_desc,
        .string_descriptor_count  = sizeof(hid_string_desc) / sizeof(hid_string_desc[0]),
        .external_phy             = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_config_desc,
        .hs_configuration_descriptor = hid_config_desc,
        .qualifier_descriptor        = NULL,
#else
        .configuration_descriptor = hid_config_desc,
#endif
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB HID keyboard ready");
}
