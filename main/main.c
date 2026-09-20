#include "bsp_audio.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "esp_log.h"
#include "worry_seed_app.h"

static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "烦恼种子启动");
    ESP_ERROR_CHECK(bsp_i2c_init());
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示或 LVGL 初始化失败");
        return;
    }
    bsp_display_backlight(82);
    esp_err_t audio_err = bsp_audio_init();
    if (audio_err != ESP_OK) ESP_LOGW(TAG, "音频初始化失败: %s", esp_err_to_name(audio_err));
    esp_err_t app_err = worry_seed_app_start();
    if (app_err != ESP_OK) ESP_LOGE(TAG, "应用启动失败: %s", esp_err_to_name(app_err));
}
