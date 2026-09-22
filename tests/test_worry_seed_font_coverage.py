#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sources = [ROOT / "main/worry_seed_app.c", ROOT / "main/worry_seed_model.c", ROOT / "main/main.c"]
required = {ch for path in sources for ch in path.read_text(encoding="utf-8")
            if ord(ch) > 0x7E and ch.isprintable()}
manifest = set((ROOT / "assets/fonts/worry-seed-symbols.txt").read_text(encoding="utf-8"))
missing = sorted(required - manifest)
assert not missing, "font subset is missing: " + "".join(missing)
for size in (16, 24):
    generated = ROOT / f"main/worry_font_{size}.c"
    assert generated.exists() and generated.stat().st_size > 1000, f"invalid {generated.name}"
    generated_text = generated.read_text(encoding="utf-8")
    absent = [f"U+{ord(ch):04X}" for ch in sorted(required)
              if f"U+{ord(ch):04X}" not in generated_text]
    assert not absent, f"font {size}px is missing glyphs: {', '.join(absent)}"
print(f"worry_seed_font_coverage: PASS ({len(required)} non-ASCII glyphs and punctuation)")
