#!/usr/bin/env python3
"""Extract the semantic layers from the supplied composite theme boards.

The supplied boards are presentation sheets, not firmware sprites.  This tool
keeps the source files untouched and produces small RGB565A8 sprites for the
five growth states and the common controls.  The alpha mask is derived from
the board background so the assets can be layered over the runtime UI.
"""

from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
from typing import Iterable

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/themes/source/01_五种风格独立图"
DERIVED = ROOT / "assets/themes/derived"
FIRMWARE = ROOT / "assets/themes/firmware"


# Coordinates are deliberately kept here, next to the source-board mapping.
# They are reviewed against the supplied boards and are not runtime layout
# coordinates.  Each tuple is x, y, width, height in source pixels.
BOARDS = {
    "pixel": {
        "source": "01_治愈像素风.png",
        "stage": [(20, 900, 90, 150), (115, 905, 145, 145),
                  (275, 845, 140, 215), (420, 815, 200, 265),
                  (850, 770, 300, 300)],
        "icons": {
            "water": (25, 325, 75, 105), "leaf": (105, 325, 85, 105),
            "fruit": (180, 325, 85, 105), "mic": (20, 435, 70, 105),
            "play": (100, 435, 75, 105), "stop": (180, 435, 75, 105),
            "back": (20, 535, 70, 85), "up": (90, 535, 70, 85),
            "down": (165, 535, 70, 85), "confirm": (235, 535, 70, 85),
        },
        # Scenic clearing only; the tree is a separate semantic layer.
        # Crop inside the right-hand scenic card; exclude its dark frame so
        # the 240x320 runtime background reaches the display edges.
        "hero": (1216, 320, 198, 420),
        "hero_opacity": 0.30,
        "background": (29, 42, 39),
    },
    "watercolor": {
        "source": "02_浪漫水彩风_重排导出.png",
        "stage": [(50, 675, 175, 115), (225, 675, 175, 115),
                  (405, 675, 175, 115), (220, 835, 190, 205),
                  (385, 825, 190, 215)],
        "icons": {
            "water": (715, 205, 120, 140), "leaf": (875, 205, 145, 140),
            "fruit": (1035, 205, 145, 140), "mic": (715, 355, 130, 130),
            "play": (875, 355, 130, 130), "stop": (1035, 355, 130, 130),
            "back": (715, 485, 125, 100), "up": (850, 485, 125, 100),
            "down": (975, 485, 125, 100), "confirm": (1095, 485, 125, 100),
        },
        "hero": (868, 725, 187, 325),
        "hero_opacity": 0.20,
        "background": (255, 248, 232),
    },
    "realistic": {
        "source": "03_微缩写实风_重排导出.png",
        "stage": [(50, 675, 175, 115), (225, 675, 175, 115),
                  (405, 675, 175, 115), (225, 835, 160, 205),
                  (385, 825, 190, 215)],
        "stage_opaque": True,
        "icons": {
            "water": (710, 205, 120, 140), "leaf": (875, 205, 145, 140),
            "fruit": (1035, 205, 140, 140), "mic": (710, 355, 135, 135),
            "play": (875, 355, 135, 135), "stop": (1035, 355, 135, 135),
            "back": (710, 485, 125, 100), "up": (850, 485, 125, 100),
            "down": (975, 485, 125, 100), "confirm": (1095, 485, 125, 100),
        },
        "hero": (660, 720, 270, 325),
        "hero_opacity": 0.30,
        "background": (44, 36, 24),
    },
    "ink": {
        "source": "04_东方水墨风_重排导出.png",
        "stage": [(50, 675, 175, 115), (225, 675, 175, 115),
                  (405, 675, 175, 115), (220, 835, 190, 205),
                  (385, 825, 190, 215)],
        "icons": {
            "water": (710, 205, 120, 140), "leaf": (875, 205, 145, 140),
            "fruit": (1035, 205, 140, 140), "mic": (710, 355, 135, 135),
            "play": (875, 355, 135, 135), "stop": (1035, 355, 135, 135),
            "back": (710, 485, 125, 100), "up": (850, 485, 125, 100),
            "down": (975, 485, 125, 100), "confirm": (1095, 485, 125, 100),
        },
        "hero": (870, 720, 185, 330),
        "hero_opacity": 0.20,
        "background": (243, 235, 212),
    },
    "starlight": {
        "source": "05_星空梦境风_重排导出.png",
        "stage": [(50, 675, 175, 115), (225, 675, 175, 115),
                  (405, 675, 175, 115), (220, 835, 190, 205),
                  (385, 825, 190, 215)],
        "icons": {
            "water": (710, 205, 120, 140), "leaf": (875, 205, 145, 140),
            "fruit": (1035, 205, 140, 140), "mic": (710, 355, 135, 135),
            "play": (875, 355, 135, 135), "stop": (1035, 355, 135, 135),
            "back": (710, 485, 125, 100), "up": (850, 485, 125, 100),
            "down": (975, 485, 125, 100), "confirm": (1095, 485, 125, 100),
        },
        "hero": (665, 725, 285, 285),
        "hero_opacity": 0.40,
        "background": (7, 20, 58),
    },
}


def crop_box(image: Image.Image, box: tuple[int, int, int, int]) -> Image.Image:
    x, y, w, h = box
    return image.crop((x, y, x + w, y + h)).convert("RGB")


def background_alpha(image: Image.Image, background: tuple[int, int, int]) -> Image.Image:
    """Make board-colored pixels transparent while preserving soft edges."""
    px = image.load()
    out = Image.new("RGBA", image.size)
    dst = out.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b = px[x, y]
            dist = math.sqrt((r - background[0]) ** 2 + (g - background[1]) ** 2 + (b - background[2]) ** 2)
            # A wider matte for the light paper boards, a tighter one for dark
            # boards where the art has stronger local contrast.
            threshold = 35 if sum(background) < 180 else 24
            alpha = 0 if dist <= threshold else min(255, int((dist - threshold) * 8))
            dst[x, y] = (r, g, b, alpha)
    return out


def _clean_alpha(image: Image.Image) -> Image.Image:
    """Drop matte noise and disconnected board neighbors around a sprite."""
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A").point(lambda value: value if value >= 72 else 0)
    rgba.putalpha(alpha)
    width, height = rgba.size
    pixels = alpha.load()
    seen: set[tuple[int, int]] = set()
    components: list[tuple[int, int, int, int, int]] = []
    for y in range(height):
        for x in range(width):
            if pixels[x, y] == 0 or (x, y) in seen:
                continue
            stack = [(x, y)]
            seen.add((x, y))
            count = 0
            min_x = max_x = x
            min_y = max_y = y
            while stack:
                cx, cy = stack.pop()
                count += 1
                min_x, max_x = min(min_x, cx), max(max_x, cx)
                min_y, max_y = min(min_y, cy), max(max_y, cy)
                for nx, ny in ((cx - 1, cy), (cx + 1, cy), (cx, cy - 1), (cx, cy + 1)):
                    if 0 <= nx < width and 0 <= ny < height and pixels[nx, ny] and (nx, ny) not in seen:
                        seen.add((nx, ny))
                        stack.append((nx, ny))
            components.append((count, min_x, min_y, max_x, max_y))
    keep = Image.new("L", (width, height), 0)
    keep_pixels = keep.load()
    margin_x, margin_y = max(2, width // 10), max(2, height // 10)
    for count, min_x, min_y, max_x, max_y in components:
        intersects = max_x >= margin_x and min_x < width - margin_x and max_y >= margin_y and min_y < height - margin_y
        if not intersects or count < 3:
            continue
        for yy in range(min_y, max_y + 1):
            for xx in range(min_x, max_x + 1):
                if pixels[xx, yy]:
                    keep_pixels[xx, yy] = 255
    rgba.putalpha(keep)
    return rgba


def _center_sprite(image: Image.Image, size: tuple[int, int], max_content: tuple[int, int]) -> Image.Image:
    image = _clean_alpha(image)
    bbox = image.getbbox()
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    if not bbox:
        return canvas
    subject = image.crop(bbox)
    subject.thumbnail(max_content, Image.Resampling.LANCZOS)
    canvas.alpha_composite(subject, ((size[0] - subject.width) // 2, (size[1] - subject.height) // 2))
    return canvas


def _opaque_sprite(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    return ImageOps.fit(image.convert("RGB"), size, method=Image.Resampling.LANCZOS).convert("RGBA")


def to_rgb565a8(image: Image.Image) -> bytes:
    rgba = image.convert("RGBA")
    colors = bytearray()
    alpha = bytearray()
    raw = rgba.tobytes()
    for offset in range(0, len(raw), 4):
        r, g, b, a = raw[offset:offset + 4]
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        colors += value.to_bytes(2, "little")
        alpha.append(a)
    return bytes(colors + alpha)


def write_asset(theme: str, kind: str, index: str, image: Image.Image, manifest: dict) -> None:
    derived = DERIVED / theme
    # Firmware files stay flat so ESP-IDF's generated linker symbols remain
    # unique even though every theme has the same semantic asset names.
    firmware = FIRMWARE
    derived.mkdir(parents=True, exist_ok=True)
    firmware.mkdir(parents=True, exist_ok=True)
    png_path = derived / f"{kind}_{index}.png"
    bin_path = firmware / f"{theme}_{kind}_{index}.rgb565a8"
    image.save(png_path)
    payload = to_rgb565a8(image)
    bin_path.write_bytes(payload)
    manifest[f"{kind}_{index}"] = {
        "png": str(png_path.relative_to(ROOT)),
        "firmware": str(bin_path.relative_to(ROOT)),
        "width": image.width,
        "height": image.height,
        "bytes": len(payload),
        "sha256": hashlib.sha256(payload).hexdigest(),
    }


def to_rgb565(image: Image.Image) -> bytes:
    """Encode an opaque RGB image for the existing LVGL hero descriptor."""
    rgb = image.convert("RGB")
    out = bytearray()
    for r, g, b in rgb.getdata():
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        out += value.to_bytes(2, "little")
    return bytes(out)


def write_hero(theme: str, src: Image.Image, box: tuple[int, int, int, int],
               fallback_background: tuple[int, int, int], opacity: float) -> dict:
    """Write a full-screen scenic plate without a duplicate growth tree."""
    derived = DERIVED / theme
    firmware = FIRMWARE
    derived.mkdir(parents=True, exist_ok=True)
    firmware.mkdir(parents=True, exist_ok=True)
    hero = ImageOps.fit(crop_box(src, box), (240, 320), method=Image.Resampling.LANCZOS)
    # Bake the visual opacity into the opaque RGB565 asset. Runtime style
    # opacity makes LVGL software-blend all 76,800 pixels on every redraw,
    # which is too expensive on the ESP32-C3 without PSRAM.
    base = Image.new("RGB", hero.size, fallback_background)
    hero = Image.blend(base, hero, opacity)
    png_path = derived / "hero_240x320.png"
    bin_path = firmware / f"{theme}.rgb565"
    hero.save(png_path)
    payload = to_rgb565(hero)
    bin_path.write_bytes(payload)
    return {
        "source": str((SOURCE / BOARDS[theme]["source"]).relative_to(ROOT)),
        "crop_xywh": list(box),
        "derived_png": str(png_path.relative_to(ROOT)),
        "firmware_asset": str(bin_path.relative_to(ROOT)),
        "width": hero.width,
        "height": hero.height,
        "bytes": len(payload),
        "sha256": hashlib.sha256(payload).hexdigest(),
        "fallback_background": "#%02X%02X%02X" % fallback_background,
        "baked_opacity": opacity,
    }


def main() -> None:
    all_manifest = {"format": "worry-seed-theme-layers-v1", "themes": {}}
    hero_manifest = []
    for theme, spec in BOARDS.items():
        src = Image.open(SOURCE / spec["source"]).convert("RGB")
        entry: dict = {"source": str((SOURCE / spec["source"]).relative_to(ROOT)), "stage": {}, "icon": {}}
        hero_manifest.append({
            "id": theme,
            "label": {"pixel": "治愈像素风", "watercolor": "浪漫水彩风", "realistic": "微缩写实风",
                      "ink": "东方水墨风", "starlight": "星空梦境风"}[theme],
            **write_hero(theme, src, spec["hero"], spec["background"], spec["hero_opacity"]),
        })
        # Keep the same 5-stage semantic contract as worry_seed_model.
        for stage, box in enumerate(spec["stage"]):
            source_image = crop_box(src, box)
            if spec.get("stage_opaque"):
                canvas = _opaque_sprite(source_image, (128, 108))
            else:
                canvas = _center_sprite(background_alpha(source_image, spec["background"]),
                                         (128, 108), (116, 100))
            write_asset(theme, "stage", str(stage), canvas, entry["stage"])
        for name, box in spec["icons"].items():
            canvas = _center_sprite(background_alpha(crop_box(src, box), spec["background"]),
                                    (40, 40), (34, 34))
            write_asset(theme, "icon", name, canvas, entry["icon"])
        all_manifest["themes"][theme] = entry
    (DERIVED / "theme-layers.json").write_text(json.dumps(all_manifest, ensure_ascii=False, indent=2) + "\n")
    (ROOT / "assets/themes/theme-assets.json").write_text(
        json.dumps(hero_manifest, ensure_ascii=False, indent=2) + "\n"
    )


if __name__ == "__main__":
    main()
