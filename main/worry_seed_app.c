#include "worry_seed_app.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp_battery.h"
#include "bsp_audio.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "worry_seed_audio.h"
#include "worry_seed_model.h"
#include "worry_seed_power_policy.h"
#include "worry_seed_store.h"
#include "worry_seed_theme.h"

LV_FONT_DECLARE(worry_font_16);
LV_FONT_DECLARE(worry_font_24);

#define INK 0x26382E
#define PAPER 0xF6F1DE
#define GREEN 0x567D5C
#define LIGHT_GREEN 0xB8C99B
#define SOIL 0xA7774D
#define CORAL 0xD77A61
#define EVENT_QUEUE_DEPTH 12
#define DETAIL_ACTION_COUNT 6

static const char *TAG = "worry_app";

typedef enum {
    PAGE_GARDEN,
    PAGE_CATEGORY,
    PAGE_RECORD,
    PAGE_DETAIL,
    PAGE_FRUIT_CONFIRM,
    PAGE_DELETE_CONFIRM,
} page_t;

typedef enum { EVENT_BUTTON, EVENT_REMOTE_BUTTON, EVENT_AUDIO } event_type_t;
typedef struct {
    event_type_t type;
    union {
        struct { bsp_btn_t button; bsp_btn_ev_t action; } key;
        struct { worry_audio_event_t action; uint16_t id; uint16_t seconds; } audio;
    } data;
} app_event_t;

typedef struct {
    worry_seed_model_t model;
    page_t page;
    uint8_t category;
    uint8_t detail_action;
    uint16_t recording_id;
    uint16_t recording_seconds;
    bool recording_update;
    bool cancel_recording;
    bool battery_ready;
    char message[48];
} app_state_t;

static QueueHandle_t s_queue;
static app_state_t s;
static lv_obj_t *s_screen;
static lv_obj_t *s_category_screen;
static lv_obj_t *s_category_rows[WORRY_CATEGORY_COUNT];
static lv_obj_t *s_category_markers[WORRY_CATEGORY_COUNT];
static worry_power_policy_t s_power;
static bool s_wake_key_pending;
static bsp_btn_t s_wake_key;
RTC_DATA_ATTR static int s_retained_battery_soc = -1;

static uint64_t now_ms(void) {
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static void log_sleep_step(const char *step, esp_err_t error) {
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "deep sleep continues after %s failed: %s", step,
                 esp_err_to_name(error));
    }
}

static void enter_deep_sleep(void) {
    if (!bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "cannot stop LVGL before automatic deep sleep");
        worry_power_policy_note_activity(&s_power, now_ms());
        bsp_display_backlight(WORRY_POWER_ACTIVE_BRIGHTNESS);
        return;
    }

    const gpio_config_t wake_input = {
        .pin_bit_mask = 1ULL << BSP_BTN_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t error = gpio_config(&wake_input);
    if (error == ESP_OK) {
        error = esp_deep_sleep_enable_gpio_wakeup(
            1ULL << BSP_BTN_GPIO, ESP_GPIO_WAKEUP_GPIO_LOW);
    }
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "button wake configuration failed: %s",
                 esp_err_to_name(error));
        bsp_lvgl_unlock();
        worry_power_policy_note_activity(&s_power, now_ms());
        bsp_display_backlight(WORRY_POWER_ACTIVE_BRIGHTNESS);
        return;
    }

    ESP_LOGI(TAG, "idle for 5 minutes; entering deep sleep until a function key");
    // CW2017 and ES8311 share I2C, so all register writes and readbacks must
    // finish before the bus pins are released. No audio I/O is active here.
    int soc = bsp_battery_soc();
    if (soc > 0 && soc <= 100) s_retained_battery_soc = soc;
    log_sleep_step("CW2017 suspend", bsp_battery_sleep());
    log_sleep_step("ES8311 suspend", bsp_audio_sleep());
    log_sleep_step("I2S pin release", bsp_audio_prepare_deep_sleep());
    log_sleep_step("shared I2C pin release", bsp_i2c_prepare_deep_sleep());
    log_sleep_step("ST7789 suspend", bsp_display_prepare_deep_sleep());

    esp_deep_sleep_start();
    ESP_LOGE(TAG, "esp_deep_sleep_start returned unexpectedly; restarting");
    esp_restart();
}

static void update_power_state(void) {
    worry_power_state_t previous = s_power.state;
    worry_power_state_t next = worry_power_policy_update(
        &s_power, now_ms(), worry_seed_audio_busy());
    if (next == previous) return;

    switch (next) {
        case WORRY_POWER_ACTIVE:
            bsp_display_backlight(WORRY_POWER_ACTIVE_BRIGHTNESS);
            break;
        case WORRY_POWER_DIMMED:
            ESP_LOGI(TAG, "idle for 30 seconds; dimming display");
            bsp_display_backlight(WORRY_POWER_DIM_BRIGHTNESS);
            break;
        case WORRY_POWER_SCREEN_OFF:
            ESP_LOGI(TAG, "idle for 60 seconds; turning backlight off");
            bsp_display_backlight(0);
            break;
        case WORRY_POWER_DEEP_SLEEP:
            enter_deep_sleep();
            break;
    }
}

static bool consume_wake_button(bsp_btn_t button, bsp_btn_ev_t action) {
    if (s_wake_key_pending) {
        if (button == s_wake_key && action != BSP_BTN_PRESS) {
            s_wake_key_pending = false;
        }
        return true;
    }

    worry_power_state_t previous = s_power.state;
    bool consume = worry_power_policy_wake_for_button(&s_power, now_ms());
    if (previous != WORRY_POWER_ACTIVE) {
        bsp_display_backlight(WORRY_POWER_ACTIVE_BRIGHTNESS);
        ESP_LOGI(TAG, "display woke from idle state %d", previous);
    }
    if (consume && action == BSP_BTN_PRESS) {
        s_wake_key_pending = true;
        s_wake_key = button;
    }
    return consume;
}

// Load the new screen before releasing the old one. Keeping detached screens
// alive exhausts LVGL's small internal pool after only a few button presses,
// so every completed page transition must release its previous object tree.
static void show_screen(lv_obj_t *screen) {
    lv_obj_t *old = s_screen;
    if (old == s_category_screen) {
        s_category_screen = NULL;
        memset(s_category_rows, 0, sizeof(s_category_rows));
        memset(s_category_markers, 0, sizeof(s_category_markers));
    }
    s_screen = screen;
    lv_screen_load(screen);
    if (old && old != screen) lv_obj_delete(old);
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                       uint32_t color, lv_align_t align, int x, int y) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(obj, align, x, y);
    return obj;
}

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_radius(obj, radius, 0);
    return obj;
}

static lv_obj_t *theme_image(lv_obj_t *parent, const lv_image_dsc_t *asset,
                             int x, int y, uint32_t scale) {
    if (!asset) return NULL;
    lv_obj_t *image = lv_image_create(parent);
    lv_image_set_src(image, asset);
    lv_image_set_scale(image, scale);
    lv_obj_set_pos(image, x, y);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE);
    return image;
}

static lv_obj_t *theme_icon(lv_obj_t *parent, worry_theme_icon_t icon,
                            int x, int y, uint32_t scale) {
    const worry_theme_t *theme = worry_seed_theme_current();
    if (icon >= WORRY_THEME_ICON_COUNT) return NULL;
    return theme_image(parent, theme->icons[icon], x, y, scale);
}

static lv_obj_t *theme_icon_centered(lv_obj_t *parent, worry_theme_icon_t icon,
                                     int center_x, int center_y, uint32_t scale) {
    // Generated semantic icons use a 40x40 canvas. LVGL's default transform
    // pivot is the unscaled image center, so positioning by the scaled bounds
    // shifts enlarged icons up and left. Anchor the original 20,20 pivot.
    return theme_icon(parent, icon, center_x - 20, center_y - 20, scale);
}

static void draw_tree(lv_obj_t *screen, worry_growth_stage_t stage) {
    const worry_theme_t *theme = worry_seed_theme_current();
    lv_obj_t *tree = lv_obj_create(screen);
    lv_obj_remove_style_all(tree);
    lv_obj_set_pos(tree, 54, 118);
    lv_obj_set_size(tree, 128, 108);
    // Keep the transparent stage asset as a plain image. Rounded clipping and
    // a timer-driven bob animation force repeated software redraws on the
    // no-PSRAM target and can starve taskLVGL while a button event is pending.
    theme_image(tree, theme->growth[stage], 0, 0, LV_SCALE_NONE);
}

static const char *stage_tip(worry_growth_stage_t stage) {
    static const char *const tips[] = {
        "种下烦恼，用心浇灌",
        "一点点改变，也会发芽",
        "坚持浇灌，烦恼正在变轻",
        "你走过的路，都在长成力量",
        "恭喜！旧烦恼变为成长的果实",
    };
    return stage <= WORRY_GROWTH_FRUIT ? tips[stage] : tips[0];
}

static lv_obj_t *screen_base(const char *title) {
    const worry_theme_t *theme = worry_seed_theme_current();
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(theme->paper), 0);
    lv_obj_t *hero = lv_image_create(screen);
    lv_image_set_src(hero, theme->hero);
    lv_obj_set_pos(hero, 0, 0);
    lv_obj_set_size(hero, 240, 320);
    lv_obj_set_style_opa(hero, theme->hero_opa, 0);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_CLICKABLE);
    label(screen, title, &worry_font_16, theme->ink, LV_ALIGN_TOP_LEFT, 14, 14);
    char battery[12] = "电量 --";
    if (s.battery_ready) {
        int soc = bsp_battery_soc();
        // CW2017 temporarily reports 0 while recomputing SOC after wake. Keep
        // the last valid pre-sleep percentage until a new reading arrives.
        if (soc == 0 && s_retained_battery_soc > 0) soc = s_retained_battery_soc;
        else if (soc > 0 && soc <= 100) s_retained_battery_soc = soc;
        if (soc >= 0) snprintf(battery, sizeof(battery), "%d%%", soc);
    }
    label(screen, battery, &worry_font_16, theme->primary, LV_ALIGN_TOP_RIGHT, -14, 14);
    return screen;
}

static void footer(lv_obj_t *screen, const char *text) {
    const worry_theme_t *theme = worry_seed_theme_current();
    box(screen, 0, 282, 240, 38, theme->footer, 0);
    label(screen, text, &worry_font_16, theme->ink, LV_ALIGN_BOTTOM_MID, 0, -10);
}

static void update_category_selection(void) {
    const worry_theme_t *theme = worry_seed_theme_current();
    for (uint8_t i = 0; i < WORRY_CATEGORY_COUNT; ++i) {
        if (!s_category_rows[i] || !s_category_markers[i]) continue;
        lv_obj_set_style_bg_color(s_category_rows[i],
                                  lv_color_hex(i == s.category ? theme->primary : theme->secondary), 0);
        lv_obj_set_style_bg_color(s_category_markers[i],
                                  lv_color_hex(i == s.category ? theme->accent : theme->ink), 0);
    }
}

static void render_garden(void) {
    const worry_theme_t *theme = worry_seed_theme_current();
    lv_obj_t *screen = screen_base("烦恼种子");
    if (!s.model.count) {
        draw_tree(screen, WORRY_GROWTH_SEED);
        label(screen, "这里还没有烦恼", &worry_font_16, theme->ink, LV_ALIGN_TOP_MID, 0, 52);
        label(screen, "说出来，埋入土里", &worry_font_16, theme->primary, LV_ALIGN_TOP_MID, 0, 76);
        label(screen, stage_tip(WORRY_GROWTH_SEED), &worry_font_16, theme->accent, LV_ALIGN_TOP_MID, 0, 244);
    } else {
        worry_seed_t *seed = &s.model.seeds[s.model.selected];
        worry_growth_stage_t stage = worry_seed_model_stage(seed);
        char line[40];
        snprintf(line, sizeof(line), "%s种子  %02u", worry_seed_category_label(seed->category), seed->id);
        label(screen, line, &worry_font_16, theme->ink, LV_ALIGN_TOP_MID, 0, 52);
        snprintf(line, sizeof(line), "%s · %u次浇灌", worry_seed_stage_label(stage), seed->update_count);
        label(screen, line, &worry_font_16, theme->primary, LV_ALIGN_TOP_MID, 0, 76);
        draw_tree(screen, stage);
        label(screen, stage_tip(stage), &worry_font_16, theme->accent, LV_ALIGN_TOP_MID, 0, 244);
        if (s.model.count > 1U) {
            snprintf(line, sizeof(line), "%u / %u", s.model.selected + 1U, s.model.count);
            label(screen, line, &worry_font_16, theme->ink, LV_ALIGN_TOP_RIGHT, -14, 76);
        }
    }
    footer(screen, "上下选择种子  长按确定播种");
    show_screen(screen);
}

static void render_category(void) {
    const worry_theme_t *theme = worry_seed_theme_current();
    // Navigation must not rebuild the whole page. Apart from being faster,
    // reusing the rows keeps the old hero image and its animation out of the
    // 24 KB LVGL pool while a selection is being changed.
    if (s_screen == s_category_screen && s_category_screen) {
        update_category_selection();
        return;
    }
    lv_obj_t *screen = screen_base("这颗种子属于");
    label(screen, "埋下一颗烦恼，等它成长", &worry_font_16, theme->accent, LV_ALIGN_TOP_MID, 0, 48);
    for (uint8_t i = 0; i < WORRY_CATEGORY_COUNT; ++i) {
        uint32_t color = i == s.category ? theme->primary : theme->secondary;
        lv_obj_t *row = box(screen, 20, 72 + i * 34, 200, 30, color, 14);
        // Keep the focus marker deliberately simple and in the row's safe area.
        // The former theme icons were easy to misread and added unnecessary
        // LVGL objects while a new category screen was being built.
        lv_obj_t *marker = box(row, 9, 9, 12, 12,
                               i == s.category ? theme->accent : theme->ink, 6);
        label(row, worry_seed_category_label(i), &worry_font_16,
              theme->ink, LV_ALIGN_LEFT_MID, 30, 0);
        s_category_rows[i] = row;
        s_category_markers[i] = marker;
    }
    s_category_screen = screen;
    footer(screen, "上下选择  确定继续  长按返回");
    show_screen(screen);
}

static void render_record(void) {
    const worry_theme_t *theme = worry_seed_theme_current();
    lv_obj_t *screen = screen_base(s.recording_update ? "记录新的进展" : "把烦恼说出来");
    label(screen, s.recording_update ? "每一次进展，都值得被听见" : "把心里的话，交给这颗种子",
          &worry_font_16, theme->accent, LV_ALIGN_TOP_MID, 0, 50);
    box(screen, 72, 78, 96, 96, s.recording_seconds ? theme->accent : theme->primary, 48);
    theme_icon_centered(screen, s.recording_seconds ? WORRY_THEME_ICON_STOP : WORRY_THEME_ICON_MIC,
                        120, 126, 360);
    label(screen, s.recording_seconds ? "正在聆听" : "准备好了", &worry_font_24,
          theme->ink, LV_ALIGN_TOP_MID, 0, 180);
    char timer[16];
    snprintf(timer, sizeof(timer), "%02u / 60秒", s.recording_seconds);
    label(screen, timer, &worry_font_16, theme->ink, LV_ALIGN_TOP_MID, 0, 208);
    label(screen, s.message[0] ? s.message :
          (s.recording_seconds ? "慢慢说，我在这里听" : "愿意说出来，就是勇敢的一步"),
          &worry_font_16, theme->accent, LV_ALIGN_TOP_MID, 0, 238);
    footer(screen, s.recording_seconds ? "确定完成  长按取消" : "确定开始  长按返回");
    show_screen(screen);
}

static const char *const ACTIONS[] = {
    "听最初的烦恼", "听最近的进展", "记录新的进展",
    "问题已经解决", "删除这颗种子", "返回种子园"
};

static void render_detail(void) {
    const worry_theme_t *theme = worry_seed_theme_current();
    lv_obj_t *screen = screen_base("陪它长大");
    worry_seed_t *seed = &s.model.seeds[s.model.selected];
    label(screen, "每一次选择，都是在照料它", &worry_font_16, theme->accent, LV_ALIGN_TOP_MID, 0, 49);
    char info[48];
    snprintf(info, sizeof(info), "%s · %s · %u次浇灌", worry_seed_category_label(seed->category),
             worry_seed_stage_label(worry_seed_model_stage(seed)), seed->update_count);
    label(screen, info, &worry_font_16, theme->primary, LV_ALIGN_TOP_MID, 0, 69);
    for (uint8_t i = 0; i < DETAIL_ACTION_COUNT; ++i) {
        uint32_t color = i == s.detail_action ? theme->primary : theme->secondary;
        lv_obj_t *row = box(screen, 21, 88 + i * 28, 198, 24, color, 12);
        worry_theme_icon_t icon = i == 2 ? WORRY_THEME_ICON_MIC
            : i == 3 ? WORRY_THEME_ICON_CONFIRM
            : i == 4 ? WORRY_THEME_ICON_STOP
            : i == 5 ? WORRY_THEME_ICON_BACK : WORRY_THEME_ICON_PLAY;
        theme_icon_centered(row, icon, 16, 12, 150);
        label(row, ACTIONS[i], &worry_font_16, theme->ink,
              LV_ALIGN_CENTER, 10, 0);
    }
    if (s.message[0]) label(screen, s.message, &worry_font_16, theme->accent, LV_ALIGN_BOTTOM_MID, 0, -44);
    footer(screen, "上下选择  确定执行  长按返回");
    show_screen(screen);
}

static void render_confirm(bool deleting) {
    const worry_theme_t *theme = worry_seed_theme_current();
    lv_obj_t *screen = screen_base(deleting ? "删除种子？" : "问题解决了吗？");
    label(screen, deleting ? "放下也需要勇气" : "你走过的路，正在开花", &worry_font_16,
          theme->accent, LV_ALIGN_TOP_MID, 0, 58);
    label(screen, deleting ? "录音也会一起删除" : "它会长成果实，留在花园里", &worry_font_16,
          deleting ? theme->accent : theme->primary, LV_ALIGN_TOP_MID, 0, 81);
    box(screen, 86, 108, 68, 68, deleting ? theme->accent : theme->primary, 34);
    theme_icon_centered(screen, deleting ? WORRY_THEME_ICON_STOP : WORRY_THEME_ICON_CONFIRM,
                        120, 142, 300);
    label(screen, "确定执行 · 长按取消", &worry_font_16, theme->ink, LV_ALIGN_TOP_MID, 0, 192);
    footer(screen, deleting ? "此操作无法恢复" : "以后仍然可以回来看它");
    show_screen(screen);
}

static void render(void) {
    int64_t started_us = esp_timer_get_time();
    ESP_LOGI(TAG, "render begin: page=%d heap=%u", s.page,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    if (!bsp_lvgl_lock(500)) {
        ESP_LOGE(TAG, "render lock timeout: page=%d", s.page);
        return;
    }
    switch (s.page) {
        case PAGE_GARDEN: render_garden(); break;
        case PAGE_CATEGORY: render_category(); break;
        case PAGE_RECORD: render_record(); break;
        case PAGE_DETAIL: render_detail(); break;
        case PAGE_FRUIT_CONFIRM: render_confirm(false); break;
        case PAGE_DELETE_CONFIRM: render_confirm(true); break;
    }
    bsp_lvgl_unlock();
    ESP_LOGI(TAG, "render done: page=%d elapsed=%lldms heap=%u", s.page,
             (long long)((esp_timer_get_time() - started_us) / 1000),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

static void set_message(const char *text) {
    snprintf(s.message, sizeof(s.message), "%s", text ? text : "");
}

static void begin_recording(bool update) {
    s.recording_update = update;
    s.recording_id = worry_seed_model_allocate_recording_id(&s.model);
    s.recording_seconds = 0;
    s.cancel_recording = false;
    set_message("");
    s.page = PAGE_RECORD;
}

static void remove_seed_audio(const worry_seed_t *seed) {
    worry_seed_store_audio_remove(seed->origin.recording_id);
    for (uint8_t i = 0; i < seed->update_count; ++i)
        worry_seed_store_audio_remove(seed->updates[i].recording_id);
}

static void handle_garden(bsp_btn_t button, bsp_btn_ev_t action) {
    if (action == BSP_BTN_LONG && button == BSP_BTN_OK) {
        if (s.model.count >= WORRY_SEED_MAX_SEEDS) { set_message("花园已满"); render(); return; }
        s.category = 0; s.page = PAGE_CATEGORY;
    } else if (action == BSP_BTN_CLICK && button == BSP_BTN_OK && s.model.count) {
        s.detail_action = 0; s.page = PAGE_DETAIL;
    } else if (action == BSP_BTN_CLICK && s.model.count) {
        if (button == BSP_BTN_UP) worry_seed_model_select(&s.model, -1);
        else if (button == BSP_BTN_DOWN) worry_seed_model_select(&s.model, 1);
    }
    render();
}

static void handle_category(bsp_btn_t button, bsp_btn_ev_t action) {
    if (action == BSP_BTN_LONG && button == BSP_BTN_OK) s.page = PAGE_GARDEN;
    else if (action == BSP_BTN_CLICK && button == BSP_BTN_UP)
        s.category = (s.category + WORRY_CATEGORY_COUNT - 1U) % WORRY_CATEGORY_COUNT;
    else if (action == BSP_BTN_CLICK && button == BSP_BTN_DOWN)
        s.category = (s.category + 1U) % WORRY_CATEGORY_COUNT;
    else if (action == BSP_BTN_CLICK && button == BSP_BTN_OK) begin_recording(false);
    render();
}

static void handle_record(bsp_btn_t button, bsp_btn_ev_t action) {
    if (button != BSP_BTN_OK) return;
    if (action == BSP_BTN_LONG) {
        if (worry_seed_audio_busy()) { s.cancel_recording = true; worry_seed_audio_stop(); set_message("正在取消…"); }
        else s.page = s.recording_update ? PAGE_DETAIL : PAGE_CATEGORY;
    } else if (action == BSP_BTN_CLICK) {
        if (worry_seed_audio_busy()) worry_seed_audio_stop();
        else {
            (void)worry_seed_audio_tone(WORRY_AUDIO_TONE_VOICE);
            if (!worry_seed_audio_record(s.recording_id)) {
                set_message("录音暂时不可用");
                render();
                return;
            }
            set_message("愿意说出来，就是勇敢的一步");
        }
    }
    render();
}

static void handle_detail(bsp_btn_t button, bsp_btn_ev_t action) {
    if (action == BSP_BTN_LONG && button == BSP_BTN_OK) { s.page = PAGE_GARDEN; set_message(""); render(); return; }
    if (action == BSP_BTN_CLICK && button == BSP_BTN_UP)
        s.detail_action = (s.detail_action + DETAIL_ACTION_COUNT - 1U) % DETAIL_ACTION_COUNT;
    else if (action == BSP_BTN_CLICK && button == BSP_BTN_DOWN)
        s.detail_action = (s.detail_action + 1U) % DETAIL_ACTION_COUNT;
    else if (button == BSP_BTN_OK) {
        if (action != BSP_BTN_CLICK) return;
        worry_seed_t *seed = &s.model.seeds[s.model.selected];
        set_message("");
        if (s.detail_action == 0) {
            if (!worry_seed_audio_play(seed->origin.recording_id)) set_message("播放暂时不可用");
            else set_message("正在播放，确定可停止");
        } else if (s.detail_action == 1) {
            if (!seed->update_count) set_message("先记录一次进展");
            else if (!worry_seed_audio_play(seed->updates[seed->update_count - 1U].recording_id)) set_message("播放暂时不可用");
            else set_message("正在播放进展，确定停止");
        } else if (s.detail_action == 2) {
            if (seed->status == WORRY_SEED_FRUITED) set_message("这颗种子已经结果");
            else if (seed->update_count >= WORRY_SEED_MAX_UPDATES) set_message("浇灌记录已满");
            else begin_recording(true);
        } else if (s.detail_action == 3) {
            if (seed->status == WORRY_SEED_FRUITED) set_message("这颗种子已经结果");
            else s.page = PAGE_FRUIT_CONFIRM;
        } else if (s.detail_action == 4) s.page = PAGE_DELETE_CONFIRM;
        else s.page = PAGE_GARDEN;
    }
    render();
}

static void handle_confirm(bsp_btn_t button, bsp_btn_ev_t action) {
    if (button != BSP_BTN_OK) return;
    if (action == BSP_BTN_LONG) s.page = PAGE_DETAIL;
    else if (action == BSP_BTN_CLICK && s.page == PAGE_FRUIT_CONFIRM) {
        worry_seed_model_t before = s.model;
        worry_seed_model_mark_fruited(&s.model, s.model.selected);
        if (worry_seed_store_save(&s.model)) {
            s.page = PAGE_GARDEN;
            (void)worry_seed_audio_tone(WORRY_AUDIO_TONE_FRUIT);
        }
        else { s.model = before; s.page = PAGE_DETAIL; set_message("保存失败，请重试"); }
    } else if (action == BSP_BTN_CLICK) {
        worry_seed_t copy = s.model.seeds[s.model.selected];
        worry_seed_model_t before = s.model;
        worry_seed_model_remove(&s.model, s.model.selected);
        if (worry_seed_store_save(&s.model)) { remove_seed_audio(&copy); s.page = PAGE_GARDEN; }
        else { s.model = before; s.page = PAGE_DETAIL; set_message("保存失败，请重试"); }
    }
    render();
}

static void handle_button(bsp_btn_t button, bsp_btn_ev_t action) {
    // UP/DOWN navigate on the low-latency press event. The button component
    // deliberately waits 180 ms before SINGLE_CLICK and converts two quick
    // presses into DOUBLE_CLICK; using CLICK made fast navigation disappear.
    // Ignore their later aggregate event so one physical press moves once.
    if (button == BSP_BTN_UP || button == BSP_BTN_DOWN) {
        if (action != BSP_BTN_PRESS) return;
        action = BSP_BTN_CLICK;
    } else if (action == BSP_BTN_PRESS || action == BSP_BTN_DOUBLE) {
        return;
    }
    if (s.page != PAGE_RECORD && worry_seed_audio_busy() && button == BSP_BTN_OK && action == BSP_BTN_CLICK) {
        worry_seed_audio_stop(); set_message("已停止播放"); render(); return;
    }
    switch (s.page) {
        case PAGE_GARDEN: handle_garden(button, action); break;
        case PAGE_CATEGORY: handle_category(button, action); break;
        case PAGE_RECORD: handle_record(button, action); break;
        case PAGE_DETAIL: handle_detail(button, action); break;
        case PAGE_FRUIT_CONFIRM:
        case PAGE_DELETE_CONFIRM: handle_confirm(button, action); break;
    }
}

static void handle_audio(const app_event_t *event) {
    worry_audio_event_t action = event->data.audio.action;
    if (action == WORRY_AUDIO_EVENT_RECORDING_STARTED) { s.recording_seconds = 1; set_message(""); }
    else if (action == WORRY_AUDIO_EVENT_RECORDING_PROGRESS) s.recording_seconds = event->data.audio.seconds;
    else if (action == WORRY_AUDIO_EVENT_RECORDING_FINISHED) {
        if (s.cancel_recording) {
            worry_seed_store_audio_remove(event->data.audio.id);
            s.cancel_recording = false;
            s.page = s.recording_update ? PAGE_DETAIL : PAGE_CATEGORY;
        } else {
            worry_seed_model_t before = s.model;
            bool ok = s.recording_update
                ? worry_seed_model_add_update(&s.model, s.model.selected, event->data.audio.id, event->data.audio.seconds)
                : worry_seed_model_add(&s.model, s.category, event->data.audio.id, event->data.audio.seconds) >= 0;
            if (ok && worry_seed_store_save(&s.model)) {
                s.page = PAGE_GARDEN;
                if (!s.recording_update) (void)worry_seed_audio_tone(WORRY_AUDIO_TONE_PLANT);
            }
            else {
                s.model = before;
                worry_seed_store_audio_remove(event->data.audio.id);
                set_message("保存失败，请重试");
            }
        }
    } else if (action == WORRY_AUDIO_EVENT_ERROR) {
        if (s.cancel_recording) { s.cancel_recording = false; s.page = s.recording_update ? PAGE_DETAIL : PAGE_CATEGORY; }
        else set_message("音频失败，请重试");
    } else if (action == WORRY_AUDIO_EVENT_PLAYBACK_FINISHED) set_message("播放完成");
    else if (action == WORRY_AUDIO_EVENT_PLAYBACK_STOPPED) set_message("已停止播放");
    render();
}

static void button_callback(bsp_btn_t button, bsp_btn_ev_t action, void *user) {
    (void)user;
    if (!s_queue) return;
    app_event_t event = { .type = EVENT_BUTTON, .data.key = { button, action } };
    (void)xQueueSend(s_queue, &event, 0);
}

esp_err_t worry_seed_app_inject_button(bsp_btn_t button, bsp_btn_ev_t action) {
    if (s_queue == NULL) return ESP_ERR_INVALID_STATE;
    if ((button != BSP_BTN_UP && button != BSP_BTN_DOWN && button != BSP_BTN_OK)
        || (action != BSP_BTN_CLICK && action != BSP_BTN_LONG)) {
        return ESP_ERR_INVALID_ARG;
    }
    app_event_t event = { .type = EVENT_REMOTE_BUTTON, .data.key = { button, action } };
    return xQueueSend(s_queue, &event, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void audio_callback(worry_audio_event_t action, uint16_t id, uint16_t seconds, void *user) {
    (void)user;
    if (!s_queue) return;
    app_event_t event = { .type = EVENT_AUDIO, .data.audio = { action, id, seconds } };
    (void)xQueueSend(s_queue, &event, 0);
}

static void app_task(void *arg) {
    (void)arg;
    // The application styles every screen, label, image, and control itself.
    // LVGL's general-purpose default theme adds many unused styles to every
    // object and was the repeated taskLVGL watchdog hotspot on this C3 target.
    if (bsp_lvgl_lock(500)) {
        lv_display_set_theme(lv_display_get_default(), NULL);
        bsp_lvgl_unlock();
        ESP_LOGI(TAG, "application-owned LVGL styles enabled");
    }
    render();
    app_event_t event;
    for (;;) {
        if (xQueueReceive(s_queue, &event, pdMS_TO_TICKS(250)) != pdTRUE) {
            update_power_state();
            continue;
        }
        if (event.type == EVENT_BUTTON) {
            ESP_LOGI(TAG, "button event: key=%d action=%d",
                     event.data.key.button, event.data.key.action);
            if (consume_wake_button(event.data.key.button, event.data.key.action)) continue;
            if (event.data.key.action == BSP_BTN_PRESS) (void)worry_seed_audio_beep();
            handle_button(event.data.key.button, event.data.key.action);
        }
        else if (event.type == EVENT_REMOTE_BUTTON) {
            ESP_LOGI(TAG, "USB button event: key=%d action=%d",
                     event.data.key.button, event.data.key.action);
            if (consume_wake_button(event.data.key.button, event.data.key.action)) continue;
            (void)worry_seed_audio_beep();
            handle_button(event.data.key.button, BSP_BTN_PRESS);
            handle_button(event.data.key.button, event.data.key.action);
        }
        else {
            worry_power_state_t previous = s_power.state;
            worry_power_policy_note_activity(&s_power, now_ms());
            if (previous != WORRY_POWER_ACTIVE) {
                bsp_display_backlight(WORRY_POWER_ACTIVE_BRIGHTNESS);
            }
            handle_audio(&event);
        }
    }
}

esp_err_t worry_seed_app_start(void) {
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_GPIO) {
        s_retained_battery_soc = -1;
    }
    memset(&s, 0, sizeof(s));
    worry_power_policy_init(&s_power, now_ms());
    s_wake_key_pending = false;
    s_screen = NULL;
    if (!worry_seed_store_init(&s.model)) return ESP_FAIL;
    worry_seed_theme_init();
    s.battery_ready = bsp_battery_init() == ESP_OK;
    s_queue = xQueueCreate(EVENT_QUEUE_DEPTH, sizeof(app_event_t));
    if (!s_queue) return ESP_ERR_NO_MEM;
    esp_err_t err = worry_seed_audio_init(audio_callback, NULL);
    if (err != ESP_OK) return err;
    err = bsp_button_init(button_callback, NULL);
    if (err != ESP_OK) return err;
    if (xTaskCreate(app_task, "worry_app", 6144, NULL, 5, NULL) != pdPASS) return ESP_ERR_NO_MEM;
    return ESP_OK;
}
