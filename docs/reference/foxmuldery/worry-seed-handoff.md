<p align="right">
  <a href="worry-seed-handoff.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Worry Seed Handoff

## Delivery snapshot

- Product: **Worry Seed**, an offline voice companion for AI Passport.
- Branch: `feature/worry-seed`; base: `main` at `cd73a8a` before the feature commit.
- Target: ESP32-C3, 8 MB Flash, 240 × 320 portrait display, three ADC-ladder buttons, ES8311 audio codec, no PSRAM.
- Toolchain: ESP-IDF 5.5.3. Wi-Fi and Bluetooth are excluded from the minimal application build.
- Merged image: `build/FoloToy-AI-Passport-full.bin`, flash at `0x0`.
- Validated size: 823,824 bytes.
- SHA-256: compute after building the final checked-out commit; ESP-IDF embeds the Git version, so the digest changes when the commit changes.

The generated `build/` directory is intentionally not committed. Rebuild from committed source before flashing, or transfer the separately delivered validated image to the local workstation.

## Product behavior

Each spoken worry becomes a seed. Follow-up recordings make it grow; marking the problem resolved produces fruit.

| State | Rule | Display |
| --- | --- | --- |
| Seed | No follow-up | Seed in soil |
| Sprout | One follow-up | Small sprout |
| Sapling | Two or three follow-ups | Young tree |
| Tree | Four or five follow-ups | Full tree |
| Fruit | Marked resolved | Tree with fruit |

The garden holds up to 12 seeds. Each has one original recording and up to five progress recordings. Categories are work, relationship, family, health, and other. Resolved seeds remain reviewable. Deletion commits the model update before removing referenced audio.

## Interaction map

| Screen | UP / DOWN | OK click | OK long press |
| --- | --- | --- | --- |
| Garden | Select seed | Open seed | Plant new seed |
| Category | Select category | Continue | Return |
| Recording, idle | No action | Start | Return |
| Recording, active | No action | Finish | Cancel |
| Detail | Select action | Run action | Return to garden |
| Confirmation | No action | Confirm | Cancel |

Detail actions play the original worry, record progress, resolve, delete, or return. OK stops active playback.

## Source map

| Path | Responsibility |
| --- | --- |
| `main/main.c` | Initializes shared I2C, display/LVGL, backlight, audio BSP, and application. |
| `main/worry_seed_app.c` | Product UI, pages, button/audio events, and model/storage coordination. |
| `main/worry_seed_model.c` | Pure garden model, selection, growth rules, limits, and validation. |
| `main/worry_seed_store.c` | NVS blob, CRC, SPIFFS, atomic recording commit, and deletion. |
| `main/worry_seed_audio.c` | Non-blocking queue and worker task for capture/playback. |
| `main/worry_seed_adpcm.c` | Platform-independent IMA ADPCM codec. |
| `main/worry_font_16.c`, `main/worry_font_24.c` | Generated LVGL Chinese subset fonts. |
| `assets/fonts/` | Font source/license notes and required-glyph manifest. |
| `tests/test_worry_seed_model.c` | Host tests for garden state. |
| `tests/test_worry_seed_adpcm.c` | Codec round-trip and error-bound test. |
| `tests/test_worry_seed_font_coverage.py` | Verifies all Chinese UI glyphs are generated. |
| `partitions.csv` | 3,008 KiB factory app plus 5 MiB `worryfs`. |
| `sdkconfig.defaults` | ESP32-C3, 8 MB Flash, native USB console, LVGL, and offline defaults. |

Hardware behavior remains in the BSP; the application duplicates no pins or device addresses.

## Persistence and audio

Garden metadata uses NVS namespace `worryseed`, key `garden`. Its blob has a magic value, storage version, structure size, model, and CRC-32. Invalid data falls back to an empty garden.

Audio lives in the 5 MiB `worryfs` SPIFFS partition as `/worry/rNNNNN.wsad`. A `.tmp` file is finalized and atomically renamed after success. Audio is mono 8 kHz, 16-bit capture encoded as 4-bit IMA ADPCM, up to 60 seconds. A maximum-length file is about 240 KB plus its header, allowing about 20 maximum-length recordings; actual capacity varies. The model's theoretical 72 recording references do not imply space for 72 maximum-length files.

SPIFFS mount failure permits formatting, so important recordings must not exist only on the device. Flashing at `0x0` writes bootloader, partition table, and application without requesting a full-chip erase. An incompatible prior partition map can still make existing data unavailable or cause formatting on mount.

## Local continuation

```sh
git clone https://gitee.com/FoloToy/ai-passport.git
cd ai-passport
git switch feature/worry-seed
source /path/to/esp-idf-v5.5.3/export.sh
./tools/validate.sh
shasum -a 256 build/FoloToy-AI-Passport-full.bin
```

Suggested prompt for the local Codex thread:

> Continue local hardware debugging of Worry Seed. Read AGENTS.md, docs/README.md, and docs/reference/foxmuldery/worry-seed-handoff.md first. Preserve existing changes. Check the branch, activate ESP-IDF 5.5.3, run complete validation, and discover the AI Passport USB Serial/JTAG port read-only. Report the device and firmware digest before requesting permission to flash. Do not erase or flash without explicit approval.

After approval, discover the macOS port and flash the merged image:

```sh
ls /dev/cu.usbmodem* /dev/cu.usbserial* 2>/dev/null
python -m esptool --chip esp32c3 --port /dev/cu.usbmodemXXXX \
  --baud 460800 write_flash 0x0 build/FoloToy-AI-Passport-full.bin
```

Do not add `erase_flash` without separate authorization for data loss.

## Validation record

- Build: **PASS** — ESP-IDF 5.5.3 compiled and verified the merged image on 2026-09-20.
- Host tests: **PASS** — repository checks, existing suites, model/codec tests, partition checks, and 138-character Chinese font coverage passed.
- Device tests: **NOT RUN** — the cloud environment cannot access the user's Mac USB device.
- Unverified: physical boot/display; Chinese rendering/layout; button clicks and long presses; microphone quality; 60-second stop, early stop, and cancellation; playback; restart persistence; all growth transitions; fruit/delete flows; battery display; low-space behavior; repeated-recording stability.

## Physical acceptance

1. Flash this exact image and capture startup logs.
2. Check for missing glyphs, clipping, overlap, and display corruption.
3. Exercise every category and all navigation/return paths.
4. Test short, manually stopped, cancelled, and full 60-second recordings.
5. Play the original recording and stop it early.
6. Add five updates and confirm Seed → Sprout → Sapling → Tree.
7. Resolve and confirm fruit persists after restart.
8. Delete and confirm metadata/audio stay removed after restart.
9. Approach the storage limit and confirm a clean error without corruption.
10. Check logs for watchdog resets, allocation, I2S, SPIFFS, or button errors.

## Known boundaries

- The app is offline: no transcription, automatic classification, sync, or upload.
- Progress recordings drive growth and are stored, but only the original recording currently has a playback action.
- There is no recording export or backup path.
- Recording requires free space for a full 60-second file, a conservative and predictable policy.
- The tree uses lightweight LVGL geometry; refine visuals only after physical review.
- Hardware acceptance is next. Delay feature expansion until capture, playback, persistence, fonts, and button timing pass.
