<p align="right">
  <a href="worry-seed-screenshot.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Worry-seed USB screenshot and debug controls

The worry-seed application exposes screenshot and simulated-button commands over
the ESP32-C3 USB Serial/JTAG port. They are local USB development controls and
are not exposed over a network.

## Capture the screen

`FAP_SCREENSHOT_V1` renders the active LVGL screen to a static RGB565 buffer and
streams one frame; it does not read the LCD SPI bus.

With the board connected, run:

```bash
python3 tools/capture_worry_seed_screenshot.py \
  --port /dev/cu.usbmodem1101 \
  --output /tmp/worry-seed.png
```

The command writes a 240x320 PNG. The port can be changed when macOS assigns a
different `/dev/cu.usbmodem*` name. The protocol is single-shot so it remains
safe during normal UI interaction; a host can poll it when a low-rate monitor
is useful.

The image represents the LVGL render result. It does not prove that the LCD
panel physically accepted every SPI transfer; use a camera for panel wiring,
signal-integrity, or intermittent hardware faults.

## Simulate buttons

Use the host tool to enqueue a click or long press:

```bash
python3 tools/control_worry_seed.py --port /dev/cu.usbmodem1101 up
python3 tools/control_worry_seed.py --port /dev/cu.usbmodem1101 down
python3 tools/control_worry_seed.py --port /dev/cu.usbmodem1101 ok
python3 tools/control_worry_seed.py --port /dev/cu.usbmodem1101 ok --long
```

The wire format is `FAP_KEY_V1 <UP|DOWN|OK> <CLICK|LONG>`. A valid request
returns `FAP_KEY_V1 QUEUED ...`; a rejected request returns `FAP_KEY_V1 ERROR
...`. The firmware places each request on the same application event queue used
by the physical buttons. It produces the same navigation, confirmation, and
feedback-tone behavior, while keeping USB work out of the LVGL task.

This control path verifies application behavior, not the ADC resistor ladder,
physical switches, electrical contacts, or hardware debouncing. Those still
require presses on the real device. Do not leave the serial monitor, screenshot
tool, and control tool open on the same port at the same time.
