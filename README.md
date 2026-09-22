<p align="right">
  <strong>English</strong> · <a href="README.zh_CN.md">简体中文</a>
</p>

# Worry Seed

> Speak it. Bury it in the soil. Let it bloom and bear fruit.

<p align="center">
  <img src="docs/reference/foxmuldery/worry-seed-assets/cover.webp" alt="Worry Seed promotional cover" width="480">
</p>

Worry Seed turns AI Passport into a private, fully offline voice garden. A spoken worry becomes a seed. Each follow-up recording waters it through five visual stages, and resolving the worry turns the journey into fruit that remains in the garden.

The experience does not transcribe, classify, diagnose, sync, or upload recordings. It is a reflection tool, not a promise that every worry will be solved.

## Experience

1. Long-press `OK` in the garden to plant a worry.
2. Choose work, relationships, family, health, or other.
3. Record up to 60 seconds of voice.
4. Return later to replay the original worry, replay the latest progress, or add a new progress recording.
5. Progress recordings move the seed through seed, sprout, sapling, and tree stages.
6. When the user marks the worry resolved, it becomes fruit and a celebration melody plays.

The garden stores up to 12 seeds. Each seed has one original recording and up to five progress recordings.

## Screens

| Empty garden | Category | Recording | Planted seed |
| --- | --- | --- | --- |
| <img src="docs/reference/foxmuldery/worry-seed-assets/app-01-empty-garden.png" alt="Empty garden" width="180"> | <img src="docs/reference/foxmuldery/worry-seed-assets/app-02-category.png" alt="Worry category selection" width="180"> | <img src="docs/reference/foxmuldery/worry-seed-assets/app-03-recording.png" alt="Voice recording" width="180"> | <img src="docs/reference/foxmuldery/worry-seed-assets/app-04-seed-planted.png" alt="Planted seed" width="180"> |

## Controls

| Screen | `UP` / `DOWN` | `OK` click | `OK` long press |
| --- | --- | --- | --- |
| Garden | Select a seed | Open the selected seed | Plant a new seed |
| Category | Select a category | Continue | Return |
| Recording, idle | No action | Start recording | Return |
| Recording, active | No action | Finish recording | Cancel |
| Seed detail | Select an action | Run the action | Return to the garden |
| Confirmation | No action | Confirm | Cancel |

Clicking `OK` while audio is playing stops playback. Button actions, planting, voice recording, and fruit completion provide distinct audio feedback; fruit completion uses a longer celebration melody.

## Visual scope

The current public experience intentionally uses one refined pixel-garden art direction. Background, growth-stage art, and action icons are layered independently so their layout can be tuned without changing the state model.

Watercolor, miniature-realistic, ink, and starlight assets exist as future theme material, but theme switching is deliberately disabled in the current firmware. They must not be described as user-selectable features yet.

## Privacy and storage

- Garden metadata is stored in NVS with a version and CRC.
- Recordings are stored only in the device's 5 MiB `worryfs` SPIFFS partition.
- Audio is 8 kHz mono IMA ADPCM. A 60-second recording is about 240 KB.
- New recordings are finalized through a temporary file and atomic rename.
- Wi-Fi and Bluetooth are not enabled by this application.
- There is currently no recording export or backup path. Important recordings should not exist only on the device.

## Build and merged firmware

Use ESP-IDF 5.5.3:

```sh
./tools/validate.sh --static
./tools/validate.sh --firmware
./tools/validate.sh
```

The merged image is written to `build/FoloToy-AI-Passport-full.bin` and is intended to be flashed at `0x0`. Obtain the device owner's approval before flashing. A merged flash writes the bootloader, partition table, and application; an incompatible previous partition map may make stored data unavailable or cause the recording partition to be formatted on mount.

## USB screenshots and simulated controls

The firmware exposes local USB Serial/JTAG commands for development:

```sh
python3 tools/capture_worry_seed_screenshot.py \
  --port /dev/cu.usbmodemXXXX --output /tmp/worry-seed.png

python3 tools/control_worry_seed.py --port /dev/cu.usbmodemXXXX up
python3 tools/control_worry_seed.py --port /dev/cu.usbmodemXXXX down
python3 tools/control_worry_seed.py --port /dev/cu.usbmodemXXXX ok
python3 tools/control_worry_seed.py --port /dev/cu.usbmodemXXXX ok --long
```

These tools exercise the same application event queue as the physical buttons. They verify rendering and application logic, not the LCD electrical path, ADC resistor ladder, switch contacts, speaker loudness, or microphone quality. See the [USB screenshot and debug-control guide](docs/development/engineering/worry-seed-screenshot.md).

## Validation and project notes

A successful build, a verified flash, USB screenshots, and simulated button actions are separate evidence. Complete physical acceptance still requires the real buttons, display, microphone, speaker volume, restart persistence, and storage behavior to be exercised on the target device.

- [Current handoff and acceptance record](docs/reference/foxmuldery/worry-seed-handoff.md)
- [Product and website language brief](docs/brand/worry-seed-website-product-brief.md)
- [AI Passport hardware guide](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md)
- [Repository documentation index](docs/README.md)
