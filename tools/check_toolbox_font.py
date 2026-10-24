#!/usr/bin/env python3
"""Check that the generated AI Toolbox fonts cover the application's text.

The character inventory lives next to the fonts.  Both 14 px and 20 px faces
must cover every printable character in it; the 28 px face only needs the
numeric glyphs used by the big traffic counter.

Generated LVGL fonts describe coverage with cmap ranges: tiny contiguous
ranges, or sparse ranges whose 16-bit unicode_list entries are offsets from
range_start.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FONT_DIR = ROOT / "assets" / "fonts"
INVENTORY = FONT_DIR / "toolbox_chars.txt"
DIGITS = "0123456789"

CMAP_RE = re.compile(
    r"\.range_start\s*=\s*(\d+).*?\.range_length\s*=\s*(\d+).*?"
    r"\.unicode_list\s*=\s*(\w+).*?\.list_length\s*=\s*(\d+)",
    re.S,
)
LIST_RE = re.compile(r"static const uint16_t (unicode_list_\w+)\[\]\s*=\s*\{(.*?)\};", re.S)


def font_coverage(path: Path) -> set[int]:
    text = path.read_text(encoding="utf-8")
    lists = {
        name: {int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]+)", body)}
        for name, body in LIST_RE.findall(text)
    }
    covered: set[int] = set()
    for start, length, list_name, list_len in CMAP_RE.findall(text):
        start_i = int(start)
        length_i = int(length)
        count = int(list_len)
        if list_name == "NULL" or count == 0:
            covered.update(range(start_i, start_i + length_i))
            continue
        entries = lists.get(list_name)
        if entries is None:
            raise SystemExit(f"{path.name}: cannot find {list_name}")
        for offset in entries:
            covered.add(start_i + offset)
    return covered


def main() -> int:
    if not INVENTORY.is_file():
        print(f"ERROR: missing character inventory {INVENTORY}", file=sys.stderr)
        return 1
    inventory = {
        ch
        for ch in INVENTORY.read_text(encoding="utf-8")
        if ch not in "\r\n" and ord(ch) >= 0x20
    }

    failures = 0
    for name in ("toolbox_font_12.c", "toolbox_font_14.c", "toolbox_font_20.c"):
        path = FONT_DIR / name
        if not path.is_file():
            print(f"ERROR: missing generated font {path}", file=sys.stderr)
            failures += 1
            continue
        covered = font_coverage(path)
        missing = sorted(ch for ch in inventory if ord(ch) not in covered)
        if missing:
            failures += 1
            points = ", ".join(f"U+{ord(ch):04X} {ch!r}" for ch in missing[:20])
            print(f"ERROR: {name} misses {len(missing)} glyph(s): {points}", file=sys.stderr)
        else:
            print(f"{name}: covers {len(inventory)} inventory characters")

    path = FONT_DIR / "toolbox_font_28.c"
    if not path.is_file():
        print(f"ERROR: missing generated font {path}", file=sys.stderr)
        failures += 1
    else:
        covered = font_coverage(path)
        missing = [ch for ch in DIGITS if ord(ch) not in covered]
        if missing:
            failures += 1
            print(f"ERROR: toolbox_font_28.c misses digits {missing}", file=sys.stderr)
        else:
            print("toolbox_font_28.c: covers 0-9")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
