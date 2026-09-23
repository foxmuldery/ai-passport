<p align="right">
  <a href="worry-seed-handoff.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Worry Seed Current Handoff

## Delivery snapshot

- Product: **Worry Seed**, a private offline voice garden running on AI Passport.
- Current local branch: `feature/worry-seed-experience`.
- Published GitHub branch at the time of this snapshot: `feature/worry-seed` at the original `783ba34` feature commit. The newer workspace has not been committed or pushed yet.
- Target: ESP32-C3, 8 MB Flash, 240 × 320 portrait display, three ADC-ladder buttons, ES8311 audio codec, no PSRAM.
- Toolchain: ESP-IDF 5.5.3. Wi-Fi and Bluetooth are excluded from the application build.
- Merged image: `build/FoloToy-AI-Passport-full.bin`, flashed at `0x0`.
- Current merged-image size: 3,057,696 bytes. The factory application has about 3% free space, so new embedded assets require explicit size review.
- Release digest: rebuild and compute after the final commit; ESP-IDF embeds the Git version and the digest changes with source state.

The generated `build/` directory is not a source release. Rebuild from the final committed state before distribution.

## Product language

- Promotional line: **Speak it. Bury it in the soil. Let it bloom and bear fruit.**
- The shorter in-device empty-garden line remains deliberately compact for the 240-pixel screen.
- User-facing language describes progress as **watering**, not companionship or automatic healing.
- The app does not promise that every worry will be solved and makes no medical or diagnostic claim.

## Product behavior

Each spoken worry becomes a seed. Follow-up recordings water it through five visual stages; marking the worry resolved creates fruit and plays a celebration melody.

| State | Rule | Display |
| --- | --- | --- |
| Seed | No follow-up | Seed in soil |
| Sprout | One follow-up | New sprout |
| Sapling | Two or three follow-ups | Young plant |
| Tree | Four or five follow-ups | Mature tree |
| Fruit | Marked resolved | Growth fruit |

The garden holds up to 12 seeds. Each seed has one original recording and up to five progress recordings. Categories are work, relationships, family, health, and other. The detail page can play the original worry, play the latest progress, record another progress entry, resolve, delete, or return.

## Interaction map

| Screen | `UP` / `DOWN` | `OK` click | `OK` long press |
| --- | --- | --- | --- |
| Garden | Select seed | Open selected seed | Plant a new seed |
| Category | Select category | Continue | Return |
| Recording, idle | No action | Start | Return |
| Recording, active | No action | Finish | Cancel |
| Detail | Select action | Run action | Return to garden |
| Confirmation | No action | Confirm | Cancel |

`UP` and `DOWN` navigation use the low-latency press path. The selected existing seed still opens with an `OK` click. Clicking `OK` while playback is active stops it and reports a stopped state rather than natural completion.

## Current experience implementation

- The pixel-garden background fills the 240 × 320 screen.
- Growth images and action icons use independent transparent layers.
- The microphone and stop icons are positioned from their unscaled center.
- Category selection uses a compact five-row list with a clear highlight.
- Button actions, planting, voice recording, and fruit completion have distinct tones; fruit completion uses a longer celebration sequence.
- Microphone input gain is configured to 42 dB and recording playback uses full output volume.
- The Chinese subset contains the application strings and punctuation checked by the font-coverage test.
- A local USB protocol captures the LVGL framebuffer and injects simulated button events through the normal application queue.
- After 30 seconds without activity the backlight dims to 18%; after 60 seconds it turns off. The first function-key press on a black screen only wakes the display and does not execute the original action.
- After five idle minutes, with no recording, playback, or tone active, the app enters deep sleep. The shared GPIO0 node for Up, Down, and OK wakes and restarts the application.
- The last valid battery percentage is retained before deep sleep and displayed while the CW2017 recomputes after wake, preventing a transient `0%`. Audio shutdown also avoids disabling I2S channels that are already stopped.
- The dedicated power key is controlled by the hardware power circuit and is not connected to an MCU/BSP input. Its roughly two-second hold remains a hardware shutdown; the app cannot reinterpret a short press as screen-off.

Only the pixel theme is active. Watercolor, miniature-realistic, ink, and starlight files exist as future material, but the theme setter rejects them so old NVS state cannot switch the application unexpectedly.

## Source map

| Path | Responsibility |
| --- | --- |
| `main/worry_seed_app.c` | UI, pages, copy, button/audio events, and model/storage coordination |
| `main/worry_seed_model.c` | Testable garden model, selection, stages, limits, and validation |
| `main/worry_seed_store.c` | NVS blob, CRC, SPIFFS, atomic recording commit, and deletion |
| `main/worry_seed_audio.c` | Worker task for capture, playback, stop handling, and feedback tones |
| `main/worry_seed_theme.c` | Current theme contract and single-theme enforcement |
| `main/worry_seed_theme_assets.c` | LVGL descriptors for backgrounds, growth layers, and icons |
| `main/worry_seed_debug_protocol.c` | USB command parser and queued simulated-button events |
| `main/worry_seed_screenshot.c` | LVGL snapshot framing over USB Serial/JTAG |
| `assets/themes/` | Source, derived, and firmware-ready theme material |
| `assets/fonts/` | Font source/license notes and required-glyph manifest |
| `tests/test_worry_seed_app_contract.py` | Input, copy, audio, volume, and layout regression contract |
| `tests/test_worry_seed_font_coverage.py` | Chinese UI glyph and punctuation coverage |
| `tools/capture_worry_seed_screenshot.py` | Save one 240 × 320 device framebuffer as PNG |
| `tools/control_worry_seed.py` | Send `UP`, `DOWN`, or `OK` click/long actions over USB |

Hardware facts and reusable board behavior remain in `components/bsp`; product state and screens remain in `main`.

## Persistence and audio

Garden metadata uses NVS namespace `worryseed`, key `garden`, with a magic value, storage version, structure size, model, and CRC-32. Invalid data falls back to an empty garden.

Audio lives in the 5 MiB `worryfs` SPIFFS partition as `/worry/rNNNNN.wsad`. A `.tmp` file is finalized and atomically renamed after success. Audio is mono 8 kHz, 16-bit capture encoded as 4-bit IMA ADPCM, up to 60 seconds. A maximum-length file is about 240 KB plus its header, allowing about 20 maximum-length recordings; actual capacity varies.

SPIFFS mount failure permits formatting, so important recordings must not exist only on the device. Flashing at `0x0` writes the bootloader, partition table, and application without requesting a full-chip erase. An incompatible earlier partition map can still make data unavailable or cause formatting on mount.

## Local continuation

The current work must be committed and pushed before a fresh clone can switch to `feature/worry-seed-experience`:

```sh
git clone https://github.com/foxmuldery/ai-passport.git
cd ai-passport
git switch feature/worry-seed-experience
source /path/to/esp-idf-v5.5.3/export.sh
./tools/validate.sh
shasum -a 256 build/FoloToy-AI-Passport-full.bin
```

Discovering a USB port is read-only and is not permission to flash. After explicit approval, flash the exact validated merged image without adding `erase_flash`:

```sh
python -m esptool --chip esp32c3 --port /dev/cu.usbmodemXXXX \
  --baud 460800 write_flash 0x0 build/FoloToy-AI-Passport-full.bin
```

## Validation record

- Build: **PASS** — ESP-IDF 5.5.3 built and verified the merged image after the current copy/font update.
- Host tests: **PASS** — repository static checks, model/codec/protocol suites, nine application-contract checks, and 182-character/punctuation font coverage passed.
- Device tests: **NOT RUN as complete acceptance** — a merged image was flashed at `0x0`, esptool verified the written data, the device returned over USB, and USB screenshots/simulated flows were captured. These observations do not prove every physical path.
- Unverified: current physical-button behavior under rapid repeated input; final microphone quality; final perceived speaker loudness; restart persistence across all stages; storage-limit behavior; long-duration repeated-recording stability.

## Physical acceptance still required

1. Exercise every category and every click/long-press return path with the physical buttons.
2. Test rapid repeated `UP`/`DOWN` presses and opening each planted seed.
3. Record quiet and normal speech, then compare original and latest-progress playback volume.
4. Confirm the button, planting, voice, and fruit sounds on the actual speaker.
5. Add five progress entries and verify every visual growth stage.
6. Resolve, restart, and confirm the fruit persists.
7. Delete a seed, restart, and confirm metadata and referenced audio remain removed.
8. Approach the storage limit and confirm a clean error without corrupting existing recordings.
9. Wait 30 seconds, 60 seconds, and five minutes to verify dim, screen-off, and deep sleep. Confirm that the first key only wakes the black screen and that all three function keys wake from deep sleep.

## Known boundaries

- The app is fully offline: no transcription, automatic classification, sync, upload, or AI judgement.
- There is no recording export or backup path.
- Recording requires space for a full 60-second file before it starts.
- USB screenshots represent LVGL output, not proof that every LCD SPI transfer reached the panel.
- Simulated USB controls do not validate the ADC resistor ladder, physical switches, or hardware debounce.
- Additional theme files consume Flash but are not selectable in the current experience.
