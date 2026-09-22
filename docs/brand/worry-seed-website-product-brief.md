<p align="right">
  <strong>English</strong> · <a href="worry-seed-website-product-brief.zh_CN.md">简体中文</a>
</p>

# WorrySeed Website and Product Brief

Status: proposal for website planning; no website implementation, deployment, or repository push is authorized by this document alone.

## 1. Product decision

- Chinese name: **Worry Seed** (see the Chinese companion document for the original script)
- English brand: **WorrySeed**
- Hero line: **Some worries grow roots before they bear fruit.**
- Release line: **Speak it. Bury it in the soil. Let it bloom and bear fruit.**
- Product type: an offline-first emotional time capsule and reflective audio device.
- It is not a therapy product, diagnosis tool, productivity tracker, electronic pet, or promise of guaranteed emotional recovery.
- WorrySeed is an independent reflection application; AI Passport is its first current hardware home.

The first website should introduce the concept and let visitors try a local, non-persistent growth animation. It should not pretend to record audio, sync data, run an AI diagnosis, or provide a real account system.

## 2. Website scope

### Recommended page flow

1. **Hero** — “Some worries grow roots before they bear fruit.”
2. **The metaphor** — seed, roots, sprout, tree, fruit
3. **Interactive demo** — five clickable stages; label it “Web demo; no data is saved”
4. **Hardware-native experience** — voice recording, three physical buttons, screen feedback, later playback
5. **Design principles** — offline-first, unresolved is valid, record change rather than score it
6. **Privacy and boundaries** — no login, no cloud upload, no diagnosis in P0
7. **Call to action** — “See how it grows” and “Explore the hardware experiment”

### Product copy direction

- “You do not have to solve it yet; plant it first.”
- “You do not need to understand everything today.”
- “Some answers need time to sprout.”
- “Congratulations. An old worry has become a fruit of growth.”
- “There is no failure state. A worry may keep growing without an answer.”

Avoid claims that every worry will be solved, that the product guarantees healing, or any medical or diagnostic wording.

## 3. Growth model

| Stage | Meaning | User-controlled evidence |
| --- | --- | --- |
| Seed | The worry has just been recorded | A first voice note exists |
| Roots | The worry is temporarily held rather than forced to resolve | Time passes or the user chooses to leave it |
| Sprout | The user sees a new angle | The user records “I see a little more clearly” |
| Tree | The user takes an action or gains an experience | A progress note is recorded |
| Fruit | The user's relationship with the worry has changed | The user confirms the change and records a takeaway |

There is no automatic score and no AI judgement of whether a worry is solved. Early opening, replanting, and deletion are valid. Deletion should use the softer label **Return to the soil**.

## 4. Visual direction

### P0 theme

Use one theme only for the first public page: **healing pixel art**. Do not show the internal Chinese theme label as a capsule or product label.

Recommended palette:

```text
Deep green  #1D2A27
Paper       #F6F1DE
Sprout      #82A86B
Forest      #42634E
Soil        #9B7048
Fruit       #E77B68
Highlight   #F5D98A
```

The other supplied styles—watercolor, realistic miniature, ink, and starlight—should remain future skins. A theme changes art, palette, animation parameters, and prompt copy; it must not change the interaction model or data model.

### Asset rules

- Use separate background, stage subject, effect, and control layers.
- Keep all page text as HTML/CSS or SVG text; do not rely on generated text inside raster images.
- Keep the seed/tree silhouette consistent across all stages.
- Use the 240 × 320 device crop as a visual constraint, but make the website layout responsive.
- Confirm public-use rights for supplied/generated artwork before publishing it. Until then, treat the source boards as internal working material.

## 5. Existing image materials

The repository already contains a source package and firmware-ready derived assets. The source boards are reference material; the derived files are the preferred inputs for a web prototype.

### Firmware-ready derived set

Each theme directory contains:

- `hero_240x320.png` — full device-sized hero/background crop, 240 × 320 RGB PNG
- `stage_0.png` … `stage_4.png` — five transparent growth-stage subjects, 128 × 108 RGBA PNG
- `icon_mic.png`, `icon_play.png`, `icon_stop.png`, `icon_back.png`
- `icon_up.png`, `icon_down.png`, `icon_confirm.png`, `icon_water.png`, `icon_leaf.png`, `icon_fruit.png`

Preferred P0 web paths:

- [Pixel hero](../../assets/themes/derived/pixel/hero_240x320.png)
- [Pixel stage 0](../../assets/themes/derived/pixel/stage_0.png)
- [Pixel stage 1](../../assets/themes/derived/pixel/stage_1.png)
- [Pixel stage 2](../../assets/themes/derived/pixel/stage_2.png)
- [Pixel stage 3](../../assets/themes/derived/pixel/stage_3.png)
- [Pixel stage 4](../../assets/themes/derived/pixel/stage_4.png)
- [Pixel microphone icon](../../assets/themes/derived/pixel/icon_mic.png)
- [Pixel play icon](../../assets/themes/derived/pixel/icon_play.png)
- [Pixel stop icon](../../assets/themes/derived/pixel/icon_stop.png)
- [Pixel navigation and action icons](../../assets/themes/derived/pixel/)

### Future theme paths

- [Watercolor](../../assets/themes/derived/watercolor/)
- [Realistic miniature](../../assets/themes/derived/realistic/)
- [Ink](../../assets/themes/derived/ink/)
- [Starlight](../../assets/themes/derived/starlight/)

### Original source boards

The original boards, historical overviews, inspection images, art direction, and source manifest are grouped under the [theme source directory](../../assets/themes/source/). The Chinese companion document lists the original folder names.

The authoritative asset inventory and checksums are maintained in:

- [`assets/themes/README.md`](../../assets/themes/README.md)
- [`assets/themes/theme-assets.json`](../../assets/themes/theme-assets.json)
- [`assets/themes/derived/theme-layers.json`](../../assets/themes/derived/theme-layers.json)

## 6. Hardware facts to expose publicly

The current project baseline is:

- ESP32-C3
- 8 MB Flash, no PSRAM
- 240 × 320 portrait display
- microphone and speaker through the existing audio BSP
- three physical buttons: Up, Down, OK
- offline-first interaction

These facts should be presented as the current experiment baseline. Verify the exact board revision and product photographs before publishing a final hardware specification.

## 7. Engineering and release decisions

- Target website repository: `foxmuldery/worrybottle-page`
- Proposed branch: `feature/worry-seed-page`
- P0 implementation: static HTML/CSS/JavaScript
- No React/Vite migration unless separately approved
- No external font dependency; use system/web-safe Chinese fallbacks
- No analytics by default
- No PWA claim unless offline caching and installation are actually implemented
- Deployment host, domain, public GitHub link, and push timing remain separate approvals

## 8. Acceptance checklist

- Mobile and desktop layouts have no horizontal overflow.
- All five stages can be selected and have visible feedback.
- Every image loads from a repository-relative path.
- Chinese copy renders correctly on common systems.
- No old theme-name capsule or obsolete brand copy appears.
- No medical, diagnostic, or guaranteed-recovery claims appear.
- The demo clearly says that it does not save data.
- Artwork ownership or license status is recorded before public release.
- Website claims are separated from firmware and physical-device acceptance.

## 9. Next approval gate

After the visual direction and copy are confirmed, create the website branch and implement the first static page. Do not flash firmware, change the device application, push to a remote, or deploy a public site as part of this document.
