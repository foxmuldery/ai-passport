#!/usr/bin/env python3
"""Send simulated button actions to the worry-seed device over USB."""

from __future__ import annotations

import argparse
import os
import re
import select
import sys
import termios
import time


ACK_RE = re.compile(rb"FAP_KEY_V1 (QUEUED|ERROR)(?: ([A-Z]+) ([A-Z]+)| ([A-Z0-9_]+))?\n")


def command_bytes(key: str, long_press: bool = False) -> bytes:
    normalized = key.upper()
    if normalized not in {"UP", "DOWN", "OK"}:
        raise ValueError(f"unsupported key: {key}")
    action = "LONG" if long_press else "CLICK"
    return f"FAP_KEY_V1 {normalized} {action}\n".encode("ascii")


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


def _read_ack(fd: int, timeout: float) -> re.Match[bytes]:
    deadline = time.monotonic() + timeout
    data = bytearray()
    while time.monotonic() < deadline:
        wait = max(0.0, min(0.2, deadline - time.monotonic()))
        readable, _, _ = select.select([fd], [], [], wait)
        if not readable:
            continue
        block = os.read(fd, 4096)
        if block:
            data.extend(block)
            match = ACK_RE.search(data)
            if match:
                return match
            if len(data) > 65536:
                del data[:-128]
    raise TimeoutError("timed out waiting for FAP_KEY_V1 acknowledgement")


def send_button(port: str, key: str, long_press: bool, timeout: float) -> None:
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        _configure_port(fd)
        os.write(fd, command_bytes(key, long_press))
        match = _read_ack(fd, timeout)
        status = match.group(1).decode("ascii")
        if status != "QUEUED":
            reason = (match.group(4) or b"UNKNOWN").decode("ascii")
            raise RuntimeError(f"device rejected button command: {reason}")
        print(f"queued {match.group(2).decode('ascii')} {match.group(3).decode('ascii')}")
    finally:
        os.close(fd)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("key", choices=("up", "down", "ok"))
    parser.add_argument("--long", action="store_true", dest="long_press")
    parser.add_argument("--port", default="/dev/cu.usbmodem1101")
    parser.add_argument("--timeout", type=float, default=3.0)
    args = parser.parse_args()
    try:
        send_button(args.port, args.key, args.long_press, args.timeout)
    except (OSError, RuntimeError, TimeoutError, ValueError) as exc:
        print(f"button command failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
