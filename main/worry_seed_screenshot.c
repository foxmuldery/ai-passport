#include "worry_seed_screenshot.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp_display.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "worry_seed_app.h"
#include "worry_seed_debug_protocol.h"

#define SCREENSHOT_WIDTH 240
#define SCREENSHOT_HEIGHT 320
#define SCREENSHOT_BYTES (SCREENSHOT_WIDTH * SCREENSHOT_HEIGHT * 2)
#define SCREENSHOT_CHUNK 512
#define SCREENSHOT_TASK_STACK 8192
#define SCREENSHOT_TASK_PRIORITY 3
#define COMMAND_LINE_BYTES 64

static const char *TAG = "worry_screenshot";

static DRAM_ATTR uint8_t s_pixels[SCREENSHOT_BYTES] __attribute__((aligned(64)));
static lv_draw_buf_t s_draw_buf;
static TaskHandle_t s_task;

static bool usb_driver_ready(void) {
    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t config = {
            .tx_buffer_size = 1024,
            .rx_buffer_size = 256,
        };
        esp_err_t err = usb_serial_jtag_driver_install(&config);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "USB screenshot driver unavailable: %s", esp_err_to_name(err));
            return false;
        }
    }
    usb_serial_jtag_vfs_use_driver();
    return usb_serial_jtag_is_driver_installed();
}

static bool usb_write_all(const uint8_t *data, size_t length) {
    while (length > 0) {
        size_t chunk = length < SCREENSHOT_CHUNK ? length : SCREENSHOT_CHUNK;
        int written = usb_serial_jtag_write_bytes(data, chunk, pdMS_TO_TICKS(1000));
        if (written != (int)chunk) return false;
        data += chunk;
        length -= chunk;
    }
    return true;
}

static bool capture_frame(void) {
    bool locked = bsp_lvgl_lock(1000);
    if (!locked) return false;

    lv_obj_t *screen = lv_screen_active();
    lv_result_t result = screen == NULL
        ? LV_RESULT_INVALID
        : lv_snapshot_take_to_draw_buf(screen, LV_COLOR_FORMAT_RGB565, &s_draw_buf);
    bool valid = result == LV_RESULT_OK
        && s_draw_buf.header.w == SCREENSHOT_WIDTH
        && s_draw_buf.header.h == SCREENSHOT_HEIGHT
        && s_draw_buf.header.cf == LV_COLOR_FORMAT_RGB565
        && s_draw_buf.header.stride == SCREENSHOT_WIDTH * 2
        && s_draw_buf.data == s_pixels;
    bsp_lvgl_unlock();
    if (!valid) return false;

    char header[96];
    int header_len = snprintf(header, sizeof(header),
                              "FAP_SCREENSHOT_V1 %d %d RGB565LE %d\n",
                              SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, SCREENSHOT_BYTES);
    if (header_len <= 0 || (size_t)header_len >= sizeof(header)) return false;

    // 日志与二进制图像共用 USB 流；从头部开始到最后一个像素期间静音日志。
    esp_log_level_set("*", ESP_LOG_NONE);
    bool sent = usb_write_all((const uint8_t *)header, (size_t)header_len)
        && usb_write_all(s_draw_buf.data, SCREENSHOT_BYTES);
    if (sent) usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(2000));
    esp_log_level_set("*", ESP_LOG_INFO);
    return sent;
}

static const char *debug_key_name(worry_debug_key_t key) {
    if (key == WORRY_DEBUG_KEY_UP) return "UP";
    if (key == WORRY_DEBUG_KEY_DOWN) return "DOWN";
    return "OK";
}

static const char *debug_action_name(worry_debug_action_t action) {
    return action == WORRY_DEBUG_ACTION_LONG ? "LONG" : "CLICK";
}

static void send_key_status(const worry_debug_command_t *command, esp_err_t result) {
    char response[80];
    int length;
    if (result == ESP_OK) {
        length = snprintf(response, sizeof(response), "FAP_KEY_V1 QUEUED %s %s\n",
                          debug_key_name(command->key), debug_action_name(command->action));
    } else {
        length = snprintf(response, sizeof(response), "FAP_KEY_V1 ERROR %s\n",
                          esp_err_to_name(result));
    }
    if (length > 0 && (size_t)length < sizeof(response)) {
        (void)usb_write_all((const uint8_t *)response, (size_t)length);
    }
}

static void dispatch_command(const char *line) {
    worry_debug_command_t command;
    if (!worry_debug_protocol_parse(line, &command)) {
        static const uint8_t error[] = "FAP_DEBUG_V1 ERROR INVALID_COMMAND\n";
        (void)usb_write_all(error, sizeof(error) - 1);
        return;
    }
    if (command.type == WORRY_DEBUG_COMMAND_SCREENSHOT) {
        (void)capture_frame();
        return;
    }

    bsp_btn_t button = BSP_BTN_OK;
    if (command.key == WORRY_DEBUG_KEY_UP) button = BSP_BTN_UP;
    else if (command.key == WORRY_DEBUG_KEY_DOWN) button = BSP_BTN_DOWN;
    bsp_btn_ev_t action = command.action == WORRY_DEBUG_ACTION_LONG
        ? BSP_BTN_LONG : BSP_BTN_CLICK;
    send_key_status(&command, worry_seed_app_inject_button(button, action));
}

static void screenshot_task(void *arg) {
    (void)arg;
    char line[COMMAND_LINE_BYTES];
    size_t line_len = 0;
    bool line_overflow = false;
    uint8_t rx[32];

    while (true) {
        if (!usb_driver_ready()) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        int count = usb_serial_jtag_read_bytes(rx, sizeof(rx), pdMS_TO_TICKS(200));
        if (count <= 0) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        for (int i = 0; i < count; ++i) {
            uint8_t byte = rx[i];
            if (byte == '\r' || byte == '\n') {
                if (line_len > 0 || line_overflow) {
                    if (line_overflow) {
                        static const uint8_t error[] = "FAP_DEBUG_V1 ERROR LINE_TOO_LONG\n";
                        (void)usb_write_all(error, sizeof(error) - 1);
                    } else {
                        line[line_len] = '\0';
                        dispatch_command(line);
                    }
                }
                line_len = 0;
                line_overflow = false;
            } else if (!line_overflow) {
                if (line_len + 1 < sizeof(line)) line[line_len++] = (char)byte;
                else line_overflow = true;
            }
        }
    }
}

esp_err_t worry_seed_screenshot_start(void) {
    if (s_task != NULL) return ESP_OK;

    if (lv_draw_buf_init(&s_draw_buf, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT,
                         LV_COLOR_FORMAT_RGB565, SCREENSHOT_WIDTH * 2,
                         s_pixels, sizeof(s_pixels)) != LV_RESULT_OK) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t created = xTaskCreate(screenshot_task, "worry_screenshot",
                                     SCREENSHOT_TASK_STACK, NULL,
                                     SCREENSHOT_TASK_PRIORITY, &s_task);
    return created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
