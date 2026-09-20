#include "worry_seed_app.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "worry_seed_audio.h"
#include "worry_seed_model.h"
#include "worry_seed_store.h"

LV_FONT_DECLARE(worry_font_16);
LV_FONT_DECLARE(worry_font_24);

#define INK 0x26382E
#define PAPER 0xF6F1DE
#define GREEN 0x567D5C
#define LIGHT_GREEN 0xB8C99B
#define SOIL 0xA7774D
#define CORAL 0xD77A61
#define EVENT_QUEUE_DEPTH 12

typedef enum {
    PAGE_GARDEN,
    PAGE_CATEGORY,
    PAGE_RECORD,
    PAGE_DETAIL,
    PAGE_FRUIT_CONFIRM,
    PAGE_DELETE_CONFIRM,
} page_t;

typedef enum { EVENT_BUTTON, EVENT_AUDIO } event_type_t;
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

static void draw_tree(lv_obj_t *screen, worry_growth_stage_t stage) {
    box(screen, 36, 224, 168, 5, SOIL, 3);
    if (stage == WORRY_GROWTH_SEED) {
        box(screen, 106, 207, 28, 16, SOIL, 12);
        return;
    }
    int trunk_h = stage == WORRY_GROWTH_SPROUT ? 25 : stage == WORRY_GROWTH_SAPLING ? 52 : 78;
    box(screen, 116, 222 - trunk_h, 8, trunk_h, SOIL, 4);
    int crown_y = stage == WORRY_GROWTH_SPROUT ? 181 : stage == WORRY_GROWTH_SAPLING ? 142 : 104;
    int crown_w = stage == WORRY_GROWTH_SPROUT ? 50 : stage == WORRY_GROWTH_SAPLING ? 92 : 132;
    int crown_h = stage == WORRY_GROWTH_SPROUT ? 32 : stage == WORRY_GROWTH_SAPLING ? 58 : 88;
    box(screen, 120 - crown_w / 2, crown_y, crown_w, crown_h, GREEN, crown_h / 2);
    if (stage == WORRY_GROWTH_FRUIT) {
        box(screen, 76, 138, 14, 14, CORAL, 7);
        box(screen, 145, 126, 14, 14, CORAL, 7);
        box(screen, 132, 168, 14, 14, CORAL, 7);
    }
}

static lv_obj_t *screen_base(const char *title) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(PAPER), 0);
    label(screen, title, &worry_font_24, INK, LV_ALIGN_TOP_LEFT, 18, 18);
    char battery[12] = "电量 --";
    if (s.battery_ready) {
        int soc = bsp_battery_soc();
        if (soc >= 0) snprintf(battery, sizeof(battery), "%d%%", soc);
    }
    label(screen, battery, &worry_font_16, GREEN, LV_ALIGN_TOP_RIGHT, -18, 23);
    return screen;
}

static void footer(lv_obj_t *screen, const char *text) {
    box(screen, 0, 282, 240, 38, GREEN, 0);
    label(screen, text, &worry_font_16, 0xFFFFFF, LV_ALIGN_BOTTOM_MID, 0, -10);
}

static void render_garden(void) {
    lv_obj_t *screen = screen_base("烦恼种子");
    if (!s.model.count) {
        draw_tree(screen, WORRY_GROWTH_SEED);
        label(screen, "这里还没有烦恼", &worry_font_16, INK, LV_ALIGN_TOP_MID, 0, 82);
        label(screen, "说出来，让它慢慢长大", &worry_font_16, GREEN, LV_ALIGN_TOP_MID, 0, 112);
    } else {
        worry_seed_t *seed = &s.model.seeds[s.model.selected];
        worry_growth_stage_t stage = worry_seed_model_stage(seed);
        char line[40];
        snprintf(line, sizeof(line), "%s种子  %02u", worry_seed_category_label(seed->category), seed->id);
        label(screen, line, &worry_font_16, INK, LV_ALIGN_TOP_MID, 0, 66);
        snprintf(line, sizeof(line), "%s · %u次陪伴", worry_seed_stage_label(stage), seed->update_count);
        label(screen, line, &worry_font_16, GREEN, LV_ALIGN_TOP_MID, 0, 91);
        draw_tree(screen, stage);
        snprintf(line, sizeof(line), "%u / %u", s.model.selected + 1U, s.model.count);
        label(screen, line, &worry_font_16, INK, LV_ALIGN_BOTTOM_MID, 0, -47);
    }
    footer(screen, s.model.count ? "上下切换  确定查看  长按播种" : "长按确定，种下第一颗");
    lv_screen_load(screen);
}

static void render_category(void) {
    lv_obj_t *screen = screen_base("这颗种子属于");
    for (uint8_t i = 0; i < WORRY_CATEGORY_COUNT; ++i) {
        uint32_t color = i == s.category ? GREEN : LIGHT_GREEN;
        lv_obj_t *row = box(screen, 30, 61 + i * 40, 180, 32, color, 14);
        label(row, worry_seed_category_label(i), &worry_font_16,
              i == s.category ? 0xFFFFFF : INK, LV_ALIGN_CENTER, 0, 0);
    }
    footer(screen, "上下选择  确定继续  长按返回");
    lv_screen_load(screen);
}

static void render_record(void) {
    lv_obj_t *screen = screen_base(s.recording_update ? "记录新的进展" : "把烦恼说出来");
    box(screen, 61, 80, 118, 118, s.recording_seconds ? CORAL : GREEN, 59);
    label(screen, s.recording_seconds ? "正在聆听" : "准备好了", &worry_font_24,
          0xFFFFFF, LV_ALIGN_TOP_MID, 0, 118);
    char timer[16];
    snprintf(timer, sizeof(timer), "%02u / 60秒", s.recording_seconds);
    label(screen, timer, &worry_font_16, INK, LV_ALIGN_TOP_MID, 0, 216);
    if (s.message[0]) label(screen, s.message, &worry_font_16, CORAL, LV_ALIGN_TOP_MID, 0, 245);
    footer(screen, s.recording_seconds ? "确定完成  长按取消" : "确定开始  长按返回");
    lv_screen_load(screen);
}

static const char *const ACTIONS[] = { "听最初的烦恼", "记录新的进展", "问题已经解决", "删除这颗种子", "返回种子园" };

static void render_detail(void) {
    lv_obj_t *screen = screen_base("陪它长大");
    worry_seed_t *seed = &s.model.seeds[s.model.selected];
    char info[48];
    snprintf(info, sizeof(info), "%s · %s · %u次陪伴", worry_seed_category_label(seed->category),
             worry_seed_stage_label(worry_seed_model_stage(seed)), seed->update_count);
    label(screen, info, &worry_font_16, GREEN, LV_ALIGN_TOP_MID, 0, 58);
    for (uint8_t i = 0; i < 5; ++i) {
        uint32_t color = i == s.detail_action ? GREEN : LIGHT_GREEN;
        lv_obj_t *row = box(screen, 21, 88 + i * 35, 198, 29, color, 12);
        label(row, ACTIONS[i], &worry_font_16, i == s.detail_action ? 0xFFFFFF : INK,
              LV_ALIGN_CENTER, 0, 0);
    }
    if (s.message[0]) label(screen, s.message, &worry_font_16, CORAL, LV_ALIGN_BOTTOM_MID, 0, -44);
    footer(screen, "上下选择  确定执行  长按返回");
    lv_screen_load(screen);
}

static void render_confirm(bool deleting) {
    lv_obj_t *screen = screen_base(deleting ? "删除种子？" : "问题解决了吗？");
    label(screen, deleting ? "录音也会一起删除" : "它会长成果实，留在花园里", &worry_font_16,
          deleting ? CORAL : GREEN, LV_ALIGN_CENTER, 0, -24);
    label(screen, "确定确认  长按取消", &worry_font_16, INK, LV_ALIGN_CENTER, 0, 22);
    footer(screen, deleting ? "此操作无法恢复" : "以后仍然可以回来看它");
    lv_screen_load(screen);
}

static void render(void) {
    if (!bsp_lvgl_lock(500)) return;
    switch (s.page) {
        case PAGE_GARDEN: render_garden(); break;
        case PAGE_CATEGORY: render_category(); break;
        case PAGE_RECORD: render_record(); break;
        case PAGE_DETAIL: render_detail(); break;
        case PAGE_FRUIT_CONFIRM: render_confirm(false); break;
        case PAGE_DELETE_CONFIRM: render_confirm(true); break;
    }
    bsp_lvgl_unlock();
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
    } else if (action == BSP_BTN_CLICK && s.model.count) {
        if (button == BSP_BTN_UP) worry_seed_model_select(&s.model, -1);
        else if (button == BSP_BTN_DOWN) worry_seed_model_select(&s.model, 1);
        else if (button == BSP_BTN_OK) { s.detail_action = 0; s.page = PAGE_DETAIL; }
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
        else if (!worry_seed_audio_record(s.recording_id)) set_message("录音暂时不可用");
        else set_message("正在启动…");
    }
    render();
}

static void handle_detail(bsp_btn_t button, bsp_btn_ev_t action) {
    if (action == BSP_BTN_LONG && button == BSP_BTN_OK) { s.page = PAGE_GARDEN; set_message(""); render(); return; }
    if (action != BSP_BTN_CLICK) return;
    if (button == BSP_BTN_UP) s.detail_action = (s.detail_action + 4U) % 5U;
    else if (button == BSP_BTN_DOWN) s.detail_action = (s.detail_action + 1U) % 5U;
    else if (button == BSP_BTN_OK) {
        worry_seed_t *seed = &s.model.seeds[s.model.selected];
        set_message("");
        if (s.detail_action == 0) {
            if (!worry_seed_audio_play(seed->origin.recording_id)) set_message("播放暂时不可用");
            else set_message("正在播放，确定可停止");
        } else if (s.detail_action == 1) {
            if (seed->status == WORRY_SEED_FRUITED) set_message("这颗种子已经结果");
            else if (seed->update_count >= WORRY_SEED_MAX_UPDATES) set_message("陪伴记录已满");
            else begin_recording(true);
        } else if (s.detail_action == 2) {
            if (seed->status == WORRY_SEED_FRUITED) set_message("这颗种子已经结果");
            else s.page = PAGE_FRUIT_CONFIRM;
        } else if (s.detail_action == 3) s.page = PAGE_DELETE_CONFIRM;
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
        if (worry_seed_store_save(&s.model)) s.page = PAGE_GARDEN;
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
            if (ok && worry_seed_store_save(&s.model)) s.page = PAGE_GARDEN;
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
    render();
}

static void button_callback(bsp_btn_t button, bsp_btn_ev_t action, void *user) {
    (void)user;
    if (!s_queue) return;
    app_event_t event = { .type = EVENT_BUTTON, .data.key = { button, action } };
    (void)xQueueSend(s_queue, &event, 0);
}

static void audio_callback(worry_audio_event_t action, uint16_t id, uint16_t seconds, void *user) {
    (void)user;
    if (!s_queue) return;
    app_event_t event = { .type = EVENT_AUDIO, .data.audio = { action, id, seconds } };
    (void)xQueueSend(s_queue, &event, 0);
}

static void app_task(void *arg) {
    (void)arg;
    render();
    app_event_t event;
    for (;;) if (xQueueReceive(s_queue, &event, portMAX_DELAY) == pdTRUE) {
        if (event.type == EVENT_BUTTON) handle_button(event.data.key.button, event.data.key.action);
        else handle_audio(&event);
    }
}

esp_err_t worry_seed_app_start(void) {
    memset(&s, 0, sizeof(s));
    if (!worry_seed_store_init(&s.model)) return ESP_FAIL;
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
