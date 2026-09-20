<p align="right">
  <strong>简体中文</strong> · <a href="worry-seed-handoff.md">English</a>
</p>

# 烦恼种子完整交接报告

## 交付快照

- 产品：**烦恼种子**，AI Passport 完全离线的语音陪伴应用。
- 分支：`feature/worry-seed`；起点为开发前的 `main` 提交 `cd73a8a`。
- 硬件：ESP32-C3、8 MB Flash、240 × 320 竖屏、ADC 电阻梯三键、ES8311 音频、无 PSRAM。
- 工具链：ESP-IDF 5.5.3；最小构建不包含 Wi-Fi 和蓝牙组件。
- 合并固件：`build/FoloToy-AI-Passport-full.bin`，从 `0x0` 刷写。
- 已验证大小：823,824 字节。
- SHA-256：应在最终提交检出并构建后计算；ESP-IDF 会嵌入 Git 版本，因此提交变化时摘要也会变化。

生成的 `build/` 目录按规范不提交。刷机前应从已提交源码重新生成固件，或者把另行交付的已验证固件传到本机。

## 产品行为

每个说出的烦恼成为一颗种子；追加进展录音推动它生长；确认问题解决后，树会结果。

| 阶段 | 规则 | 画面 |
| --- | --- | --- |
| 种子 | 0 次进展 | 土里的种子 |
| 发芽 | 1 次进展 | 小芽 |
| 幼苗 | 2–3 次进展 | 小树 |
| 成树 | 4–5 次进展 | 完整树冠 |
| 结果 | 标记问题已解决 | 树上结果 |

花园最多 12 颗种子，每颗包含 1 条初始录音和最多 5 条进展录音；分类为工作、关系、家庭、健康、其他。结果后的种子继续保留。删除时先持久化模型变化，再删除引用的录音。

## 交互表

| 页面 | 上／下 | 单击确定 | 长按确定 |
| --- | --- | --- | --- |
| 种子园 | 切换种子 | 查看种子 | 播种新种子 |
| 分类 | 选择类别 | 继续 | 返回 |
| 录音待机 | 无 | 开始 | 返回 |
| 正在录音 | 无 | 完成 | 取消 |
| 详情 | 选择动作 | 执行动作 | 返回种子园 |
| 确认页 | 无 | 确认 | 取消 |

详情页可听最初的烦恼、记录进展、标记解决、删除或返回；播放时单击确定可以停止。

## 源代码结构

| 路径 | 职责 |
| --- | --- |
| `main/main.c` | 初始化共享 I2C、显示/LVGL、背光、音频 BSP 和应用。 |
| `main/worry_seed_app.c` | 产品 UI、页面、按键/音频事件，以及模型与存储协调。 |
| `main/worry_seed_model.c` | 可独立测试的花园模型、选择、生长规则、限制和校验。 |
| `main/worry_seed_store.c` | NVS 数据块、CRC、SPIFFS、录音原子提交和删除。 |
| `main/worry_seed_audio.c` | 非阻塞队列及负责采集/播放的工作任务。 |
| `main/worry_seed_adpcm.c` | 与平台无关的 IMA ADPCM 编解码器。 |
| `main/worry_font_16.c`、`main/worry_font_24.c` | 生成的 LVGL 中文子集字体。 |
| `assets/fonts/` | 字体来源/许可与所需字形清单。 |
| `tests/test_worry_seed_model.c` | 花园状态主机测试。 |
| `tests/test_worry_seed_adpcm.c` | 编解码往返及误差边界测试。 |
| `tests/test_worry_seed_font_coverage.py` | 校验所有中文界面字形。 |
| `partitions.csv` | 3,008 KiB factory 应用区和 5 MiB `worryfs`。 |
| `sdkconfig.defaults` | ESP32-C3、8 MB Flash、原生 USB 控制台、LVGL 及离线配置。 |

硬件行为仍以 BSP 为唯一来源，应用没有复制引脚或器件地址。

## 持久化与音频

花园元数据存放在 NVS 命名空间 `worryseed`、键 `garden` 中，包含魔数、存储版本、结构大小、模型和 CRC-32；损坏或不兼容时回退为空花园。

录音位于 5 MiB 的 `worryfs` SPIFFS 分区，文件名 `/worry/rNNNNN.wsad`。先写 `.tmp`，完成并补全文件头后再原子改名。格式为 8 kHz 单声道、16 位采集、4 位 IMA ADPCM，最长 60 秒。满时长约 240 KB 加文件头，分区约容纳 20 条满时长录音；实际容量随时长变化。因此模型最多 72 个录音引用，不等于能保存 72 条满时长录音。

SPIFFS 挂载失败时允许格式化，重要录音不能只保存在设备中。从 `0x0` 刷写会写 bootloader、分区表和应用，但不主动整片擦除；旧分区布局不兼容时，既有数据仍可能不可读或因挂载失败被格式化。

## 本地线程接续

```sh
git clone https://gitee.com/FoloToy/ai-passport.git
cd ai-passport
git switch feature/worry-seed
source /path/to/esp-idf-v5.5.3/export.sh
./tools/validate.sh
shasum -a 256 build/FoloToy-AI-Passport-full.bin
```

新建本地 Codex 线程后使用：

> 继续本地调试“烦恼种子”。先完整阅读 AGENTS.md、docs/README.md 和 docs/reference/foxmuldery/worry-seed-handoff.zh_CN.md，保留现有修改。检查当前分支，激活 ESP-IDF 5.5.3，运行完整验证，并以只读方式发现 AI Passport 的 USB Serial/JTAG 串口。先报告设备和固件摘要，再申请刷机许可；未经明确确认不得擦除或刷写。

获得确认后，在 macOS 上发现串口并刷写：

```sh
ls /dev/cu.usbmodem* /dev/cu.usbserial* 2>/dev/null
python -m esptool --chip esp32c3 --port /dev/cu.usbmodemXXXX \
  --baud 460800 write_flash 0x0 build/FoloToy-AI-Passport-full.bin
```

未另行获得允许数据丢失的授权，不要添加 `erase_flash`。

## 验证记录

- Build：**PASS**——2026-09-20 使用 ESP-IDF 5.5.3 完成编译和合并固件校验。
- Host tests：**PASS**——仓库检查、既有测试、模型/编解码测试、分区检查和 138 个中文字符覆盖全部通过。
- Device tests：**NOT RUN**——云端环境无法访问用户 Mac 的 USB 设备。
- Unverified：真机启动/显示、中文渲染/排版、按键单击/长按、录音质量、60 秒停止、提前结束/取消、播放、重启持久化、生长阶段、结果/删除、电量显示、空间不足、连续录音稳定性。

## 真机验收

1. 刷入这一准确固件并保存启动日志。
2. 检查缺字方框、裁切、重叠和花屏。
3. 测试全部分类及导航/返回路径。
4. 测试短录音、手动结束、取消和完整 60 秒录音。
5. 播放初始录音并提前停止。
6. 追加 5 次进展，确认“种子→发芽→幼苗→成树”。
7. 标记解决，重启后确认结果仍在。
8. 删除种子，重启后确认元数据和录音仍被删除。
9. 接近存储上限，确认明确报错且不破坏既有数据。
10. 检查看门狗、内存、I2S、SPIFFS 和按键错误日志。

## 已知边界

- 完全离线，不做转写、自动分类、同步或上传。
- 进展录音会推动生长并保存在设备中，但当前只有初始录音提供播放入口。
- 暂无录音导出或备份通道。
- 开始录音前按完整 60 秒预留空间，策略保守但可预测。
- 树由轻量 LVGL 几何图形绘制，应在真机审查后再精修。
- 下一步是硬件验收；录音、播放、持久化、字体和按键时序通过前不宜扩展功能。
