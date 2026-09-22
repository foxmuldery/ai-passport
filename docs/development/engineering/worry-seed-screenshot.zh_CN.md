<p align="right">
  <strong>简体中文</strong> · <a href="worry-seed-screenshot.md">English</a>
</p>

# 烦恼种子 USB 截屏与调试控制

烦恼种子应用通过 ESP32-C3 的 USB Serial/JTAG 接口提供截屏和模拟按键命令。
这些命令只用于本机 USB 调试，不通过网络开放。

## 截取屏幕

`FAP_SCREENSHOT_V1` 会把当前 LVGL 页面渲染到静态 RGB565 缓冲区并发送一帧；
它不会读取 LCD 的 SPI 总线。

连接设备后运行：

```bash
python3 tools/capture_worry_seed_screenshot.py \
  --port /dev/cu.usbmodem1101 \
  --output /tmp/worry-seed.png
```

命令会生成一张 240×320 PNG。macOS 重新分配设备名时，把端口改成实际的
`/dev/cu.usbmodem*` 路径即可。协议是单次截屏，不会持续占用正常 UI 交互；
需要低帧率监看时，可以由电脑端定时轮询。

这张图代表 LVGL 的渲染结果，不能证明 LCD 面板确实接收了每一次 SPI 传输。
如果要排查面板接线、信号完整性或偶发花屏，仍需使用摄像头拍摄实体屏幕。

## 模拟按键

使用电脑端工具把单击或长按事件送入设备：

```bash
python3 tools/control_worry_seed.py --port /dev/cu.usbmodem1101 up
python3 tools/control_worry_seed.py --port /dev/cu.usbmodem1101 down
python3 tools/control_worry_seed.py --port /dev/cu.usbmodem1101 ok
python3 tools/control_worry_seed.py --port /dev/cu.usbmodem1101 ok --long
```

线路协议为 `FAP_KEY_V1 <UP|DOWN|OK> <CLICK|LONG>`。设备接受命令后返回
`FAP_KEY_V1 QUEUED ...`；拒绝时返回 `FAP_KEY_V1 ERROR ...`。固件会把每条命令
放入与实体按键共用的应用事件队列，因此页面导航、确认操作和提示音行为一致，
同时不会让 USB 任务直接操作 LVGL。

这个控制通道只能验证应用逻辑，不能验证 ADC 电阻梯、实体开关、电气接触或硬件
消抖；这些项目仍需在真机上按键测试。串口监视器、截屏工具和按键控制工具不要
同时占用同一个端口。
