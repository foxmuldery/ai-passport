<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐、电池参数等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

### AI Toolbox 字库

| 文件 | 尺寸 / 格式 | 用途与来源 |
| --- | --- | --- |
| [`fonts/toolbox_font_12.c`](fonts/toolbox_font_12.c) | 12 px，4 bpp，LVGL 9 C 数组 | 到期与榜单列表中的紧凑名称。字符集与 14 px 一致。 |
| [`fonts/toolbox_font_14.c`](fonts/toolbox_font_14.c) | 14 px，4 bpp，LVGL 9 C 数组 | AI Toolbox 应用正文。由 Noto Sans SC Regular（SIL Open Font License 1.1）生成，包含 GB2312 一级字库与应用字符清单。 |
| [`fonts/toolbox_font_20.c`](fonts/toolbox_font_20.c) | 20 px，4 bpp，LVGL 9 C 数组 | 标题与分区标题；字符集与 14 px 一致。 |
| [`fonts/toolbox_font_28.c`](fonts/toolbox_font_28.c) | 28 px，4 bpp，LVGL 9 C 数组 | 大号访问量数字；只含数字与分数标点。 |
| [`fonts/toolbox_chars.txt`](fonts/toolbox_chars.txt) | UTF-8 文本 | 应用必需字符清单，供 `tools/check_toolbox_font.py` 校验。 |

源字体：[Noto Sans SC Regular](https://github.com/notofonts/noto-cjk)，SIL Open Font License 1.1。三个 C 数组通过 `main/CMakeLists.txt` 的 `target_sources` 编译，不在其他源文件中 `#include`。改动字符清单后，用固定版本的转换器重新生成：

```bash
npx -y lv_font_conv@1.5.2 --font NotoSansCJKsc-Regular.otf \
  --symbols "$(cat assets/fonts/toolbox_chars.txt)" --size 20 --bpp 4 \
  --format lvgl --no-compress --no-kerning \
  --output assets/fonts/toolbox_font_20.c
```

12 px、14 px 与 20 px 字库覆盖字符清单并额外包含 GB2312 一级字库（3,755 个常用汉字），常见的站点名与订阅名无需重新构建即可显示。该子集不是完整 CJK 字库：清单之外的字符仍可能显示 LVGL 缺字占位符。遇到时把字符加入清单并重新生成三个字号；`tools/check_toolbox_font.py` 已加入静态门禁，会报告缺失的码点。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。

## 电池参数（battery）

[`battery/wanli-500mah-profile2-xil40x-20261020.txt`](battery/wanli-500mah-profile2-xil40x-20261020.txt)
保存万锂新能源 500 mAh 电芯的 CW2017 参数（profile2、XIL40X、2026-10-20）。
该文件是项目提供的 `喜梨_智能工卡_万锂新能源500mAh_profile2_XIL40X_20261020.txt`
的原样副本，按维护者要求保留用于本次固件更新；原文件没有附带单独的许可说明。

格式为 80 行两位十六进制数字，每行一个字节，按寄存器顺序排列。
BSP 在 `components/bsp/src/bsp_battery.c` 中嵌入这 80 字节，主机检查会验证
源文件及数组内容完全一致。文本文件 SHA-256：
`7ea96eba4634f4495a37c25ffacd54c1deaa0693b162c892d671a2ba68460919`。
解码后 80 字节 SHA-256：
`fa7365671093ef449c5bdcfb4adb279e3d275f888109f50b6b2f21e9b96220d5`。
这是电量计参数，不是充电配置，也不能通用于所有 500 mAh 电芯；详见
[硬件指南](../docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.zh_CN.md#9-cw2017-电池计)。
