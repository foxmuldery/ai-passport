[简体中文](README.zh_CN.md)

# Worry Seed font subset

The firmware embeds 16 px and 24 px subsets generated from Adobe Source Han Sans SC Medium. Source: `https://github.com/adobe-fonts/source-han-sans/raw/release/OTF/SimplifiedChinese/SourceHanSansSC-Medium.otf`, SHA-256 `1df61d31687d04fd2f928a3bb6ca6cd61f0e988cc267cf317f32406edbb49f70`.

Source Han Sans is distributed under the SIL Open Font License 1.1. The full 15.7 MB source font is intentionally not committed. To regenerate:

```sh
symbols="$(cat assets/fonts/worry-seed-symbols.txt)"
npx --yes lv_font_conv --font SourceHanSansSC-Medium.otf --range 0x20-0x7E --symbols "$symbols·…" --size 16 --bpp 2 --format lvgl --no-compress --no-kerning --lv-font-name worry_font_16 --lv-include lvgl.h --output main/worry_font_16.c
npx --yes lv_font_conv --font SourceHanSansSC-Medium.otf --range 0x20-0x7E --symbols "$symbols·…" --size 24 --bpp 2 --format lvgl --no-compress --no-kerning --lv-font-name worry_font_24 --lv-include lvgl.h --output main/worry_font_24.c
```
