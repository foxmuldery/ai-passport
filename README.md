[简体中文](README.zh_CN.md)

# Worry Seed · AI Passport

Worry Seed is a fully offline voice companion. A spoken worry becomes a seed. Each progress recording helps it grow through seed, sprout, sapling, and tree stages; resolving the problem lets the tree bear fruit.

## Controls

- Garden: Up/Down selects a seed, OK opens it, and long-press OK plants a new one.
- Planting: Choose work, relationship, family, health, or other, then record up to 60 seconds.
- Care: Replay the original worry, add up to five progress recordings, mark it resolved, or delete it.
- Metadata stays in NVS and recordings stay in the device's `worryfs` partition. Nothing is uploaded.

Audio uses 8 kHz mono IMA ADPCM. A maximum-length recording is about 240 KB. The app supports 12 seeds; the 5 MB recording partition holds about 20 maximum-length recordings, with actual capacity depending on recording length.

## Build

Use ESP-IDF 5.5.3:

```sh
./tools/validate.sh --static
./tools/validate.sh --firmware
```

The merged image is written to `build/FoloToy-AI-Passport-full.bin` and flashes at address `0x0`. Obtain the device owner's confirmation before flashing.

## Data safety

New audio is written to a temporary file and atomically renamed after completion. Garden metadata is versioned and protected by a CRC. Wi-Fi and Bluetooth are disabled. SPIFFS formats a damaged recording partition on the first failed mount, so irreplaceable recordings should not exist only on the device.
