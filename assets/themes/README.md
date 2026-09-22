[简体中文](README.zh_CN.md) · **English**

# Worry Seed Theme Assets

This directory stores the supplied art package and firmware-ready theme assets.

## Layout

- `source/`: original supplied boards, drafts, previews, and the supplied image manifest.
- `derived/`: 240 x 320 hero crops plus extracted, transparent stage and icon PNGs.
- `firmware/`: RGB565 background images and RGB565A8 semantic layers embedded into the ESP32-C3 firmware.
- `theme-assets.json`: background crop coordinates, source mapping, byte sizes, and SHA-256 values.
- `derived/theme-layers.json`: reproducible source boxes and checksums for the five growth stages and ten controls per theme.

The five runtime themes are `pixel`, `watercolor`, `realistic`, `ink`, and `starlight`. The firmware keeps the interaction model unchanged and switches the palette, hero art, and layout copy together.

On the device, long-press Up or Down on the garden page to switch themes. The selected theme is saved in NVS and restored after reboot. Long-press OK keeps its existing meaning: start planting a new worry.

The supplied images are presentation boards rather than separate files. `tools/build_worry_seed_theme_assets.py` extracts their semantic layers without changing the originals. The runtime theme now supplies the hero background, five growth-stage foregrounds, and themed water/leaf/fruit/microphone/play/stop/navigation/confirm controls. Chinese copy remains device-rendered so font coverage can be checked independently of the art.
