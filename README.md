# Tanmatsu USB Keyboard

Turns a [Tanmatsu](https://docs.tanmatsu.cloud) badge into a USB HID keyboard. The device enumerates as a standard USB Full-Speed (USB 1.1) keyboard on the host PC, forwarding every key press and release from the hardware keyboard in real time.

Note: this app was generated using Claude.

## How it works

The ESP32-P4 inside Tanmatsu has two USB controllers sharing one physical port:

- **USB Serial JTAG** — used for debug/flashing (default at boot)
- **USB OTG Full-Speed (1.1)** — used by this application

On startup the firmware switches the shared PHY from the JTAG controller to the OTG 1.1 controller (`USB_DEVICE` mode), then initialises [TinyUSB](https://github.com/hathach/tinyusb) as a HID boot-keyboard device.

The BSP delivers raw PS/2 scan code set 1 events (`INPUT_EVENT_TYPE_SCANCODE`). These are translated to USB HID keycodes via a lookup table before being sent to the host in a standard 8-byte keyboard report (modifier byte + 6-key rollover).

## Building

```
make DEVICE=tanmatsu
```

Flash and monitor:

```
make flashmonitor DEVICE=tanmatsu PORT=/dev/ttyACM0
```

> **Note:** Once the firmware is running, the USB port enumerates as a keyboard and the JTAG console is no longer available over USB. Use a UART adapter for serial logs if needed.

## Key mapping

PS/2 scan code set 1 make/break codes are translated to USB HID keycodes. All standard keys are supported including:

- Full alphanumeric and symbol keys
- Modifier keys: Ctrl, Shift, Alt, GUI/Meta (left and right variants)
- Function keys F1–F12
- Navigation: arrows, Home, End, Page Up/Down, Insert, Delete
- Numpad (with Num Lock)
- Extended keys via the 0xE0 prefix

The `Fn` key (BSP-specific, scancode 0x55) has no USB HID equivalent and is silently ignored.

## License

The contents of this repository may be considered in the public domain or [CC0-1.0](https://creativecommons.org/publicdomain/zero/1.0) licensed at your disposal.

At Nicolai Electronics we love open source so we recommend licensing your work based on this template under terms of the [MIT license](https://opensource.org/license/mit).
