#!/usr/bin/env python3
"""Capture one worry-seed LVGL frame over USB Serial/JTAG and save a PNG."""

from __future__ import annotations

import argparse
import os
import re
import select
import struct
import sys
import termios
import time
import zlib


HEADER_RE = re.compile(rb"FAP_SCREENSHOT_V1 (\d+) (\d+) RGB565LE (\d+)\n")


def _png_chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)


def rgb565le_to_png(pixels: bytes, width: int, height: int) -> bytes:
    if len(pixels) != width * height * 2:
        raise ValueError("RGB565 payload length does not match its dimensions")

    rows = bytearray()
    for y in range(height):
        rows.append(0)
        row = pixels[y * width * 2 : (y + 1) * width * 2]
        for offset in range(0, len(row), 2):
            value = row[offset] | (row[offset + 1] << 8)
            red = ((value >> 11) & 0x1F) * 255 // 31
            green = ((value >> 5) & 0x3F) * 255 // 63
            blue = (value & 0x1F) * 255 // 31
            rows.extend((red, green, blue))

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + _png_chunk(b"IHDR", ihdr) + _png_chunk(b"IDAT", zlib.compress(bytes(rows), 6)) + _png_chunk(b"IEND", b"")


def _configure_port(fd: int) -> None:
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CLOCAL | termios.CREAD
    attrs[3] = 0
    attrs[4] = termios.B115200
    attrs[5] = termios.B115200
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)


def _read_until_header(fd: int, timeout: float) -> tuple[re.Match[bytes], bytes]:
    deadline = time.monotonic() + timeout
    data = bytearray()
    while time.monotonic() < deadline:
        wait = max(0.0, min(0.2, deadline - time.monotonic()))
        readable, _, _ = select.select([fd], [], [], wait)
        if not readable:
            continue
        block = os.read(fd, 4096)
        if not block:
            continue
        data.extend(block)
        match = HEADER_RE.search(data)
        if match:
            return match, bytes(data[match.end() :])
        if len(data) > 65536:
            del data[:-64]
    raise TimeoutError("timed out waiting for FAP_SCREENSHOT_V1 header")


def _read_exact(fd: int, prefix: bytes, length: int, timeout: float) -> bytes:
    result = bytearray(prefix[:length])
    deadline = time.monotonic() + timeout
    while len(result) < length and time.monotonic() < deadline:
        wait = max(0.0, min(0.2, deadline - time.monotonic()))
        readable, _, _ = select.select([fd], [], [], wait)
        if not readable:
            continue
        block = os.read(fd, length - len(result))
        if block:
            result.extend(block)
    if len(result) != length:
        raise TimeoutError(f"screenshot ended at {len(result)}/{length} bytes")
    return bytes(result)


def capture(port: str, output: str, timeout: float) -> None:
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        _configure_port(fd)
        os.write(fd, b"FAP_SCREENSHOT_V1\n")
        match, prefix = _read_until_header(fd, timeout)
        width, height, length = (int(match.group(i)) for i in (1, 2, 3))
        pixels = _read_exact(fd, prefix, length, timeout)
        with open(output, "wb") as stream:
            stream.write(rgb565le_to_png(pixels, width, height))
        print(f"saved {output} ({width}x{height}, {length} RGB565 bytes)")
    finally:
        os.close(fd)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/cu.usbmodem1101")
    parser.add_argument("--output", default="worry-seed-screenshot.png")
    parser.add_argument("--timeout", type=float, default=10.0)
    args = parser.parse_args()
    try:
        capture(args.port, args.output, args.timeout)
    except (OSError, TimeoutError, ValueError) as exc:
        print(f"capture failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
