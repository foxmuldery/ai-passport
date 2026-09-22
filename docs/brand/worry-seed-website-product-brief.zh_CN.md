<p align="right">
  <a href="worry-seed-website-product-brief.md">English</a> · <strong>简体中文</strong>
</p>

# 烦恼种子官网与项目介绍确认文档

状态：官网规划确认稿；本文档本身不授权网页实现、部署或推送。

## 1. 产品结论

- 中文名：**烦恼种子**
- 英文品牌：**WorrySeed**
- 英文主句：**Some worries grow roots before they bear fruit.**
- 中文主句：**有些烦恼，会先长根**
- 中文发布副句：**说出来，埋在土里，等它结果开花**
- 产品类型：离线优先的情绪时间胶囊与自我回听硬件。
- 它不是心理治疗产品、诊断工具、效率打卡工具、电子宠物，也不承诺保证解决情绪问题。
- WorrySeed 是独立的情绪整理应用；AI Passport 是它当前的首个硬件载体。

首版官网用于介绍理念，并提供一个本地、不保存数据的生长动画演示。不应伪装成真实录音应用、云端同步服务、AI 诊断工具或账号系统。

## 2. 官网范围

### 推荐页面结构

1. **首屏**——“有些烦恼，会先长根”
2. **产品隐喻**——种子、生根、发芽、成树、结果
3. **互动演示**——五个阶段可点击切换，并标注“网页演示，不保存数据”
4. **硬件体验**——录音、三个实体按键、屏幕反馈、之后重新回听
5. **设计原则**——离线优先、未解决也有效、记录变化而不是打分
6. **隐私与边界**——P0 不登录、不上传、不诊断
7. **行动按钮**——“看看它如何生长”和“了解硬件实验”

### 推荐文案

- “不用急着解决，先把它种下。”
- “你不需要现在就想明白。”
- “有些答案，需要一点时间发芽。”
- “恭喜！旧烦恼变为成长的果实。”
- “没有失败的状态，暂时没有答案也可以继续生长。”

避免使用“所有烦恼都会解决”“保证治愈”以及任何医疗或诊断表述。

## 3. 生长模型

| 阶段 | 含义 | 用户确认依据 |
| --- | --- | --- |
| 种子 | 刚刚记录烦恼 | 存在第一段声音记录 |
| 生根 | 暂时安放烦恼，而不是强迫自己解决 | 时间经过，或用户选择暂不处理 |
| 发芽 | 用户看见了新的角度 | 用户记录“我看清了一点” |
| 成树 | 用户采取了行动，或获得了经验 | 记录一条进展 |
| 结果 | 用户确认自己与这件事的关系发生了变化 | 确认变化并记录带走的经验 |

不设置自动分数，也不由 AI 判断烦恼是否解决。允许提前挖开、重新种下和删除；删除按钮建议使用较温和的文案：**归还土地**。

## 4. 视觉方向

### P0 主题

首个公开页面只使用一个主题：**治愈像素风**。页面中不展示“治愈像素”这个内部主题名称，也不放主题胶囊。

建议配色：

```text
深绿       #1D2A27
纸张米白   #F6F1DE
嫩芽绿     #82A86B
森林绿     #42634E
土壤色     #9B7048
果实珊瑚   #E77B68
高光暖金   #F5D98A
```

现有的浪漫水彩、微缩写实、东方水墨和星空梦境作为后续皮肤。主题只更换美术、配色、动画参数和提示文案，不改变交互逻辑和数据模型。

### 素材规则

- 背景、成长主体、效果和控件必须分层使用。
- 页面文字使用 HTML/CSS 或 SVG 真实排版，不依赖图片内生成文字。
- 所有阶段保持同一套种子、树干、树冠和果实造型。
- 以 240 × 320 设备画布作为视觉约束，但官网布局必须响应式。
- 公开发布前确认用户提供及生成素材的使用权；在此之前，原始展示板只作为内部开发素材。

## 5. 当前图像素材

仓库已经包含原始美术包和固件可用的裁切素材。原始图用于参考，网页原型优先使用裁切后的 derived 素材。

### 固件可用的裁切素材

每个主题目录包含：

- `hero_240x320.png`——240 × 320 的主视觉/背景 PNG
- `stage_0.png` 至 `stage_4.png`——五张透明成长阶段主体图，128 × 108 RGBA PNG
- `icon_mic.png`、`icon_play.png`、`icon_stop.png`、`icon_back.png`
- `icon_up.png`、`icon_down.png`、`icon_confirm.png`、`icon_water.png`、`icon_leaf.png`、`icon_fruit.png`

首版网页优先使用：

- [像素风主视觉](../../assets/themes/derived/pixel/hero_240x320.png)
- [像素风阶段 0](../../assets/themes/derived/pixel/stage_0.png)
- [像素风阶段 1](../../assets/themes/derived/pixel/stage_1.png)
- [像素风阶段 2](../../assets/themes/derived/pixel/stage_2.png)
- [像素风阶段 3](../../assets/themes/derived/pixel/stage_3.png)
- [像素风阶段 4](../../assets/themes/derived/pixel/stage_4.png)
- [像素风麦克风图标](../../assets/themes/derived/pixel/icon_mic.png)
- [像素风播放图标](../../assets/themes/derived/pixel/icon_play.png)
- [像素风停止图标](../../assets/themes/derived/pixel/icon_stop.png)
- [像素风导航与操作图标目录](../../assets/themes/derived/pixel/)

### 后续主题目录

- [浪漫水彩](../../assets/themes/derived/watercolor/)
- [微缩写实](../../assets/themes/derived/realistic/)
- [东方水墨](../../assets/themes/derived/ink/)
- [星空梦境](../../assets/themes/derived/starlight/)

### 原始素材目录

- [五种风格独立图](../../assets/themes/source/01_五种风格独立图/)
- [历次总览与初稿](../../assets/themes/source/02_历次总览与初稿/)
- [过程预览与检查图](../../assets/themes/source/03_过程预览与检查图/)
- [美术规范与原始清单](../../assets/themes/source/04_说明文档/)

素材清单和校验值维护在：

- [`assets/themes/README.zh_CN.md`](../../assets/themes/README.zh_CN.md)
- [`assets/themes/theme-assets.json`](../../assets/themes/theme-assets.json)
- [`assets/themes/derived/theme-layers.json`](../../assets/themes/derived/theme-layers.json)

## 6. 官网可公开的硬件信息

当前项目基线：

- ESP32-C3
- 8 MB Flash，无 PSRAM
- 240 × 320 竖屏
- 现有 BSP 支持的麦克风和扬声器
- 三个实体按键：上、下、确认
- 离线优先交互

这些信息可以作为当前实验基线公开。正式发布硬件规格前，仍需重新确认具体板号、硬件版本和产品照片。

## 7. 工程与发布决定

- 目标网页仓库：`foxmuldery/worrybottle-page`
- 建议分支：`feature/worry-seed-page`
- P0：静态 HTML/CSS/JavaScript
- 未经单独确认不迁移 React/Vite
- 不依赖外部字体；使用系统和网页安全中文字体回退
- 默认不接统计分析
- 未真正实现离线缓存和安装能力前，不宣称 PWA
- 部署平台、域名、公开 GitHub 链接和推送时机另行确认

## 8. 验收清单

- 手机端和桌面端没有横向溢出。
- 五个阶段都能选择并有明显反馈。
- 所有图片都从仓库相对路径加载。
- 常见系统上的中文文字完整可读。
- 不出现旧主题名胶囊或过时品牌文案。
- 不出现医疗、诊断或保证解决问题的说法。
- 演示明确说明不保存数据。
- 公开发布前记录素材版权或授权状态。
- 官网宣传、固件实现和实体设备验收分别说明。

## 9. 下一步确认点

确认视觉和文案方向后，再创建网页分支并实现首版静态页面。本文档不包含刷机、修改设备应用、远程推送或公开部署授权。
