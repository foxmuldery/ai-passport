// AI Toolbox — a FoloToy AI Passport application.
//
// Four screens over one dark dashboard:
//   站点 — Cloudflare Pages traffic for the last 7 days
//   到期 — subscription / quota deadlines, nearest first
//   榜单 — top 10 models of the selected leaderboard
//   时钟 — large local clock, date and a browsable weekly strip
//
// OK pages through the three tabs; UP/DOWN scroll inside the current tab
// (site detail, more expiry rows, ranks 6-10). Long OK refreshes, long DOWN
// cycles the leaderboard in both directions, and long UP then long OK clears the stored
// configuration and reboots into the Wi-Fi setup portal. The backlight dims
// after 20 s and turns off after 60 s without input; the first key wakes.

#include <stdlib.h>
#include <time.h>

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "key_guard.h"

#include "toolbox_net.h"
#include "toolbox_ui.h"

static const char *TAG = "tb_main";
#define KEY_GUARD_MAX_US (10LL * 1000 * 1000)

static key_guard_t s_key_guard;
static volatile bool s_input_ready;

static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    const bool was_armed = s_key_guard.armed;
    if (key_guard_consume(&s_key_guard, ev == BSP_BTN_RELEASE, esp_timer_get_time())) {
        if (was_armed && !s_key_guard.armed) {
            ESP_LOGI(TAG, "wake key released, input enabled again");
        }
        return;
    }
    if (!s_input_ready) {
        return;
    }
    // Runs on the shared esp_timer task: only enqueue, never block.
    tb_ui_key_event(btn, ev);
}

void app_main(void)
{
    setenv("TZ", "CST-8", 1);
    tzset();

    ESP_LOGI(TAG, "AI Toolbox starting");
    const uint32_t wake_causes = esp_sleep_get_wakeup_causes();
    if (wake_causes & (1u << ESP_SLEEP_WAKEUP_GPIO)) {
        key_guard_arm(&s_key_guard, esp_timer_get_time(), KEY_GUARD_MAX_US);
        ESP_LOGI(TAG, "GPIO wake: swallow keys until the held key is released");
    }

    bsp_i2c_init();
    bsp_i2c_scan();

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "display/LVGL init failed — check the panel wiring");
        return;
    }
    bsp_display_backlight(100);

    if (bsp_battery_init() != ESP_OK) {
        ESP_LOGW(TAG, "battery gauge not available; UI will show —");
    }

    if (!bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "could not lock LVGL for UI creation");
        return;
    }
    esp_err_t ui_err = tb_ui_start();
    bsp_lvgl_unlock();
    if (ui_err != ESP_OK) {
        ESP_LOGE(TAG, "UI init failed: %s", esp_err_to_name(ui_err));
        return;
    }

    esp_err_t err = bsp_button_init(on_key, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "button init failed: %s", esp_err_to_name(err));
    } else {
        if (s_key_guard.armed) {
            const bool released = bsp_button_read_mv() >= BSP_BTN_MV_RELEASED_MIN;
            key_guard_sync_released(&s_key_guard, released);
        }
        s_input_ready = true;
    }

    err = tb_net_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "network start failed: %s", esp_err_to_name(err));
    }



    ESP_LOGI(TAG, "ready, free heap=%lu", (unsigned long)esp_get_free_heap_size());
}
