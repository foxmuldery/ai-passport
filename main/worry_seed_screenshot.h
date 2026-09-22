#pragma once

#include "esp_err.h"

// 启动 USB 单次截屏协议监听任务。命令与像素流都走 USB Serial/JTAG，
// 不读取 LCD（本板 LCD 没有 MISO），因此不会占用显示 SPI 总线。
esp_err_t worry_seed_screenshot_start(void);
