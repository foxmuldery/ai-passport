#!/usr/bin/env python3
"""Host checks for the USB screenshot payload encoder."""

from __future__ import annotations

import importlib.util
import struct
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("capture", ROOT / "tools/capture_worry_seed_screenshot.py")
assert SPEC and SPEC.loader
CAPTURE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CAPTURE)

CONTROL_SPEC = importlib.util.spec_from_file_location("control", ROOT / "tools/control_worry_seed.py")
assert CONTROL_SPEC and CONTROL_SPEC.loader
CONTROL = importlib.util.module_from_spec(CONTROL_SPEC)
CONTROL_SPEC.loader.exec_module(CONTROL)


def chunks(png: bytes):
    offset = 8
    while offset < len(png):
        length = struct.unpack(">I", png[offset : offset + 4])[0]
        kind = png[offset + 4 : offset + 8]
        payload = png[offset + 8 : offset + 8 + length]
        yield kind, payload
        offset += 12 + length


def main() -> None:
    pixels = bytes((0x00, 0xF8, 0xE0, 0x07))  # red, green in RGB565LE
    png = CAPTURE.rgb565le_to_png(pixels, 2, 1)
    assert png.startswith(b"\x89PNG\r\n\x1a\n")
    image_header, image_data = list(chunks(png))[:2]
    assert image_header == (b"IHDR", struct.pack(">IIBBBBB", 2, 1, 8, 2, 0, 0, 0))
    assert image_data[0] == b"IDAT"
    assert zlib.decompress(image_data[1]) == bytes((0, 255, 0, 0, 0, 255, 0))
    assert CAPTURE.HEADER_RE.fullmatch(b"FAP_SCREENSHOT_V1 240 320 RGB565LE 153600\n")
    assert CONTROL.command_bytes("up") == b"FAP_KEY_V1 UP CLICK\n"
    assert CONTROL.command_bytes("DOWN", True) == b"FAP_KEY_V1 DOWN LONG\n"
    assert CONTROL.command_bytes("ok") == b"FAP_KEY_V1 OK CLICK\n"
    assert CONTROL.ACK_RE.search(b"log noise\nFAP_KEY_V1 QUEUED OK CLICK\n")
    assert CONTROL.ACK_RE.fullmatch(b"FAP_KEY_V1 ERROR ESP_ERR_TIMEOUT\n")
    try:
        CONTROL.command_bytes("left")
    except ValueError:
        pass
    else:
        raise AssertionError("unsupported keys must be rejected")
    print("Screenshot protocol host tests: PASS")


if __name__ == "__main__":
    main()
