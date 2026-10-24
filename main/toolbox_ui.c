#include "toolbox_ui.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "bsp_battery.h"
#include "bsp_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"

#include "toolbox_logic.h"
#include "toolbox_model.h"
#include "toolbox_net.h"
#include "toolbox_portal.h"

LV_FONT_DECLARE(toolbox_font_12);
LV_FONT_DECLARE(toolbox_font_14);
LV_FONT_DECLARE(toolbox_font_20);
LV_FONT_DECLARE(toolbox_font_28);

#define COLOR_BG 0x070B14
#define COLOR_CARD 0x0F1727
#define COLOR_LINE 0x22304A
#define COLOR_INK 0xE6EDF7
#define COLOR_MUTED 0x8FA3BF
#define COLOR_DIM 0x61748F
#define COLOR_CYAN 0x22D3EE
#define COLOR_VIOLET 0xA78BFA
#define COLOR_GREEN 0x34D399
#define COLOR_AMBER 0xFBBF24
#define COLOR_ORANGE 0xFB923C
#define COLOR_RED 0xFB7185

#define SCREEN_W 240
#define SCREEN_H 320
#define CARD_X 10
#define CARD_Y 68
#define CARD_W 220
#define CARD_H 216
#define SLEEP_DIM_US (20LL * 1000 * 1000)
#define SLEEP_OFF_US (60LL * 1000 * 1000)
#define DETAIL_PAGE 5

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
} key_event_t;

static lv_obj_t *s_main;
static lv_obj_t *s_portal_page;
static lv_obj_t *s_portal_ssid_label;
static lv_obj_t *s_time_label;
static lv_obj_t *s_net_label;
static lv_obj_t *s_net_dot;
static lv_obj_t *s_battery_label;
static lv_obj_t *s_hint_label;
static lv_obj_t *s_tab_buttons[3];
static lv_obj_t *s_tab_labels[3];
static lv_obj_t *s_pages[3];

static lv_obj_t *s_site_name;
static lv_obj_t *s_site_sub;
static lv_obj_t *s_site_pv;
static lv_obj_t *s_site_visits;
static lv_obj_t *s_site_week;
static lv_obj_t *s_site_month;

static lv_obj_t *s_bars[7];

static lv_obj_t *s_exp_title;
static lv_obj_t *s_exp_sub;
static lv_obj_t *s_exp_rows[5];
static lv_obj_t *s_exp_bars[5];
static lv_obj_t *s_exp_names[5];
static lv_obj_t *s_exp_badges[5];

static lv_obj_t *s_board_title;
static lv_obj_t *s_board_meta;
static lv_obj_t *s_board_rows[5];
static lv_obj_t *s_board_ranks[5];
static lv_obj_t *s_board_models[5];
static lv_obj_t *s_board_scores[5];

static lv_obj_t *s_portal_qr;

static QueueHandle_t s_key_queue;
static lv_timer_t *s_tick_timer;
static tb_summary_t s_data;
static bool s_have_data;
static int s_page;
static int s_board_index;
static int s_board_offset;
static int s_exp_offset;
static int s_site_index;
static int s_tick;
static bool s_sleeping;
static bool s_dimmed;
static bool s_swallow_until_release;
static int64_t s_last_activity_us;
static bool s_loading;
static bool s_reset_armed;
static bool s_portal_visible;

static lv_obj_t *box_create(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius,
                            bool transparent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, transparent ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *label_create(lv_obj_t *parent, const char *text, const lv_font_t *font,
                              uint32_t color, int x, int y, int w, int h)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, w, h);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_label_set_text(label, text);
    return label;
}

static void label_set(lv_obj_t *label, const char *text)
{
    lv_label_set_text(label, text);
}

static uint32_t tier_color(const char *tier)
{
    if (strcmp(tier, "red") == 0) {
        return COLOR_RED;
    }
    if (strcmp(tier, "orange") == 0) {
        return COLOR_ORANGE;
    }
    if (strcmp(tier, "amber") == 0) {
        return COLOR_AMBER;
    }
    if (strcmp(tier, "neutral") == 0) {
        return COLOR_CYAN;
    }
    return COLOR_DIM;
}

static void update_tabs(void)
{
    for (int i = 0; i < 3; i++) {
        const bool active = i == s_page;
        lv_obj_set_style_bg_opa(s_tab_buttons[i], active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(s_tab_buttons[i], lv_color_hex(0x14263D), 0);
        lv_obj_set_style_text_color(s_tab_labels[i],
                                    lv_color_hex(active ? COLOR_CYAN : COLOR_MUTED), 0);
    }
    for (int i = 0; i < 3; i++) {
        if (i == s_page) {
            lv_obj_clear_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void update_hint(void)
{
    if (s_portal_visible) {
        label_set(s_hint_label, "配网完成后自动重启");
        return;
    }
    if (s_sleeping) {
        label_set(s_hint_label, "按键唤醒");
        return;
    }
    if (s_reset_armed) {
        label_set(s_hint_label, "再长按 OK 重置配网");
        return;
    }
    if (s_loading) {
        label_set(s_hint_label, "正在连接服务器…");
        return;
    }
    const tb_net_state_t state = tb_net_state();
    if (state == TB_NET_WIFI_FAILED) {
        label_set(s_hint_label, "WiFi 失败 · 长按 UP 重置");
        return;
    }
    if (state == TB_NET_OFFLINE) {
        label_set(s_hint_label, "离线 · 长按 OK 重试");
        return;
    }
    switch (s_page) {
    case 2:
        if ((s_tick / 100) % 2 == 0) {
            label_set(s_hint_label, "OK翻页 · ↑↓名次 · 长按OK刷新");
        } else {
            label_set(s_hint_label, "长按↑↓换榜 · 长按OK刷新");
        }
        break;
    case 1:
        label_set(s_hint_label, "OK翻页 · ↑↓记录 · 长按OK刷新");
        break;
    default:
        label_set(s_hint_label, "OK翻页 · ↑↓站点 · 长按OK刷新");
        break;
    }
}

static void render_site(void)
{
    if (!s_have_data) {
        label_set(s_site_name, "正在读取…");
        label_set(s_site_sub, "今日浏览");
        label_set(s_site_pv, "—");
        label_set(s_site_visits, "访问 —");
        label_set(s_site_month, "30 天 —");
        for (int i = 0; i < 7; i++) {
            lv_obj_add_flag(s_bars[i], LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    if (s_site_index > s_data.site_count) {
        s_site_index = 0;
    }
    const tb_site_t *picked = NULL;
    if (s_site_index > 0 && s_site_index <= s_data.site_count) {
        picked = &s_data.sites[s_site_index - 1];
    }

    const char *name = picked ? picked->name : (s_data.site_name[0] ? s_data.site_name : "未命名站点");
    const int pv = picked ? picked->pv : s_data.pv;
    const int visits = picked ? picked->visits : s_data.visits;
    const int month = picked ? picked->month : s_data.month;
    const int errors = picked ? picked->error : s_data.site_errors;
    const int *trend = picked ? picked->trend : s_data.trend;
    const int trend_count = picked ? picked->trend_count : s_data.trend_count;

    char text[48];
    label_set(s_site_name, name);
    if (picked != NULL) {
        snprintf(text, sizeof(text), "今日浏览 · %d/%d", s_site_index, s_data.site_count);
    } else {
        snprintf(text, sizeof(text), "今日浏览 · 全部");
    }
    label_set(s_site_sub, text);

    snprintf(text, sizeof(text), "%d", pv);
    label_set(s_site_pv, text);
    snprintf(text, sizeof(text), "访问 %d", visits);
    label_set(s_site_visits, text);

    if (errors) {
        lv_obj_set_style_text_color(s_site_month, lv_color_hex(COLOR_RED), 0);
        label_set(s_site_month, "该站点取数异常");
    } else {
        lv_obj_set_style_text_color(s_site_month, lv_color_hex(COLOR_MUTED), 0);
        snprintf(text, sizeof(text), "30 天 %d PV", month);
        label_set(s_site_month, text);
    }

    const int max_value = tb_max_int(trend, trend_count);
    for (int i = 0; i < 7; i++) {
        const int index = trend_count - 7 + i;
        const int val = (index >= 0 && index < trend_count) ? trend[index] : 0;
        if (trend_count == 0) {
            lv_obj_add_flag(s_bars[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(s_bars[i], LV_OBJ_FLAG_HIDDEN);
        const int height = tb_bar_height(val, max_value, 48, 3);
        lv_obj_set_height(s_bars[i], height);
        lv_obj_set_y(s_bars[i], 170 - height);
        lv_obj_set_style_bg_color(s_bars[i],
                                  lv_color_hex(i == 6 ? COLOR_VIOLET : COLOR_CYAN), 0);
    }
}

static void render_expiry(void)
{
    if (!s_have_data) {
        label_set(s_exp_sub, "正在读取…");
        for (int i = 0; i < 5; i++) {
            lv_obj_add_flag(s_exp_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    const int max_offset = s_data.expiry_count > 5 ? s_data.expiry_count - 5 : 0;
    s_exp_offset = tb_clamp_int(s_exp_offset, 0, max_offset);

    char text[64];
    if (max_offset > 0) {
        const int last = s_exp_offset + 5 < s_data.expiry_count
                             ? s_exp_offset + 5
                             : s_data.expiry_count;
        snprintf(text, sizeof(text), "共%d · 7天%d · %d-%d",
                 s_data.expiry_count, s_data.alerts_due3, s_exp_offset + 1, last);
    } else {
        snprintf(text, sizeof(text), "共%d · 7天内%d", s_data.expiry_count, s_data.alerts_due3);
    }
    label_set(s_exp_sub, text);

    for (int i = 0; i < 5; i++) {
        const int index = s_exp_offset + i;
        if (index >= s_data.expiry_count) {
            lv_obj_add_flag(s_exp_rows[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(s_exp_rows[i], LV_OBJ_FLAG_HIDDEN);
        const tb_expiry_t *e = &s_data.expiry[index];
        label_set(s_exp_names[i], e->name);
        const char *badge = e->badge;
        if (strncmp(badge, "已过期", 9) == 0) {
            badge += 3;
        }
        label_set(s_exp_badges[i], badge);
        const uint32_t color = tier_color(e->tier);
        lv_obj_set_style_bg_color(s_exp_bars[i], lv_color_hex(color), 0);
        lv_obj_set_style_text_color(s_exp_badges[i], lv_color_hex(color), 0);
    }
}

static void render_board(void)
{
    if (!s_have_data || s_data.board_count == 0) {
        label_set(s_board_title, "模型榜单");
        label_set(s_board_meta, s_have_data ? "还没有取到榜单" : "正在读取…");
        for (int i = 0; i < 5; i++) {
            lv_obj_add_flag(s_board_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    if (s_board_index >= s_data.board_count) {
        s_board_index = 0;
    }
    const tb_board_t *board = &s_data.boards[s_board_index];
    const int max_offset = board->top_count > 5 ? board->top_count - 5 : 0;
    s_board_offset = tb_clamp_int(s_board_offset, 0, max_offset);

    label_set(s_board_title, board->name);
    char meta[80];
    if (board->error) {
        snprintf(meta, sizeof(meta), "榜%d/%d · 取数失败", s_board_index + 1, s_data.board_count);
        lv_obj_set_style_text_color(s_board_meta, lv_color_hex(COLOR_RED), 0);
    } else if (board->top_count == 0) {
        snprintf(meta, sizeof(meta), "榜%d/%d · %s · 暂无",
                 s_board_index + 1, s_data.board_count,
                 board->updated[0] ? board->updated : "—");
        lv_obj_set_style_text_color(s_board_meta, lv_color_hex(COLOR_MUTED), 0);
    } else {
        const int first = s_board_offset + 1;
        const int last = s_board_offset + 5 < board->top_count ? s_board_offset + 5 : board->top_count;
        snprintf(meta, sizeof(meta), "榜%d/%d · %s · %d-%d/%d",
                 s_board_index + 1, s_data.board_count,
                 board->updated[0] ? board->updated : "—", first, last, board->top_count);
        lv_obj_set_style_text_color(s_board_meta, lv_color_hex(COLOR_MUTED), 0);
    }
    label_set(s_board_meta, meta);

    for (int i = 0; i < 5; i++) {
        const int index = s_board_offset + i;
        if (index >= board->top_count) {
            lv_obj_add_flag(s_board_rows[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(s_board_rows[i], LV_OBJ_FLAG_HIDDEN);
        const tb_entry_t *entry = &board->top[index];
        char rank[8];
        snprintf(rank, sizeof(rank), "#%d", entry->rank);
        label_set(s_board_ranks[i], rank);
        lv_obj_set_style_text_color(s_board_ranks[i],
                                    lv_color_hex(entry->rank <= 3 ? COLOR_CYAN : COLOR_MUTED), 0);
        label_set(s_board_models[i], entry->model);
        char score[16];
        tb_format_score(entry->score, score, sizeof(score));
        label_set(s_board_scores[i], score);
    }
}

static void render_all(void)
{
    render_site();
    render_expiry();
    render_board();
    update_tabs();
    update_hint();
}

static void update_clock(void)
{
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    if (local.tm_year + 1900 < 2024) {
        label_set(s_time_label, "--:--");
        return;
    }
    char text[8];
    strftime(text, sizeof(text), "%H:%M", &local);
    label_set(s_time_label, text);
}

static void update_battery(void)
{
    const int soc = bsp_battery_soc();
    char text[8];
    if (soc < 0) {
        snprintf(text, sizeof(text), "—");
    } else {
        snprintf(text, sizeof(text), "%d%%", tb_clamp_int(soc, 0, 100));
    }
    label_set(s_battery_label, text);
    uint32_t color = COLOR_GREEN;
    if (soc < 20) {
        color = COLOR_RED;
    } else if (soc < 50) {
        color = COLOR_AMBER;
    }
    lv_obj_set_style_text_color(s_battery_label, lv_color_hex(color), 0);
}

static void update_network_indicator(void)
{
    tb_net_state_t state = tb_net_state();
    uint32_t color = COLOR_DIM;
    if (state == TB_NET_ONLINE) {
        color = COLOR_GREEN;
    } else if (state == TB_NET_CONNECTING) {
        color = COLOR_AMBER;
    } else if (state == TB_NET_OFFLINE || state == TB_NET_WIFI_FAILED) {
        color = COLOR_RED;
    } else if (state == TB_NET_PROVISION) {
        color = COLOR_VIOLET;
    }
    lv_obj_set_style_bg_color(s_net_dot, lv_color_hex(color), 0);
    label_set(s_net_label, tb_net_state_text());
}

static void show_portal(bool show)
{
    if (show == s_portal_visible) {
        return;
    }
    s_portal_visible = show;
    if (show) {
        s_last_activity_us = esp_timer_get_time();
        char ssid[48];
        snprintf(ssid, sizeof(ssid), "Wi-Fi · %s", tb_portal_ssid());
        label_set(s_portal_ssid_label, ssid);
        lv_obj_clear_flag(s_portal_page, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_main, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_portal_page, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_main, LV_OBJ_FLAG_HIDDEN);
    }
    update_hint();
}

static void switch_page(int page)
{
    s_page = (page + 3) % 3;
    s_reset_armed = false;
    update_tabs();
    update_hint();
}

static void wake_screen(void)
{
    const bool was_sleeping = s_sleeping;
    s_sleeping = false;
    s_dimmed = false;
    s_last_activity_us = esp_timer_get_time();
    bsp_display_backlight(100);
    tb_net_set_low_power(false);
    if (s_tick_timer != NULL) {
        lv_timer_set_period(s_tick_timer, 100);
    }
    if (was_sleeping) {
        render_all();
        update_clock();
        update_battery();
        update_network_indicator();
    }
}

/** UP/DOWN scrolls inside the current tab; it no longer changes tabs. */
static void scroll_detail(int direction)
{
    if (!s_have_data) {
        return;
    }
    if (s_page == 2) {
        if (s_data.board_count == 0) {
            return;
        }
        const int top_count = s_data.boards[s_board_index].top_count;
        const int max_offset = top_count > 5 ? top_count - 5 : 0;
        s_board_offset = tb_clamp_int(s_board_offset + direction * DETAIL_PAGE, 0, max_offset);
        render_board();
    } else if (s_page == 1) {
        const int max_offset = s_data.expiry_count > 5 ? s_data.expiry_count - 5 : 0;
        s_exp_offset = tb_clamp_int(s_exp_offset + direction * DETAIL_PAGE, 0, max_offset);
        render_expiry();
    } else if (s_page == 0) {
        s_site_index = tb_clamp_int(s_site_index + direction, 0, s_data.site_count);
        render_site();
    }
    update_hint();
}

static void switch_board(int direction)
{
    if (!s_have_data || s_data.board_count < 2) {
        return;
    }
    s_board_index = (s_board_index + direction + s_data.board_count) % s_data.board_count;
    s_board_offset = 0;
    render_board();
    update_hint();
}

static void handle_key(const key_event_t *event)
{
    s_last_activity_us = esp_timer_get_time();
    if (s_portal_visible) {
        return;
    }

    // The first gesture after idle only lights the screen and is swallowed
    // until release, so waking never changes a tab or triggers a refresh.
    if (s_sleeping || s_dimmed) {
        if (event->ev == BSP_BTN_RELEASE) {
            s_swallow_until_release = false;
            return;
        }
        if (!s_swallow_until_release) {
            wake_screen();
            s_swallow_until_release = true;
        }
        return;
    }
    if (s_swallow_until_release) {
        if (event->ev == BSP_BTN_RELEASE) {
            s_swallow_until_release = false;
        }
        return;
    }
    if (event->ev != BSP_BTN_CLICK && event->ev != BSP_BTN_LONG) {
        return;
    }

    if (event->ev == BSP_BTN_LONG) {
        if (event->btn == BSP_BTN_UP) {
            if (s_page == 2 && s_have_data && s_data.board_count > 1) {
                switch_board(-1);
                return;
            }
            s_reset_armed = true;
            update_hint();
            return;
        }
        if (event->btn == BSP_BTN_OK) {
            if (s_reset_armed) {
                tb_net_reset_provisioning();
                return;
            }
            s_loading = true;
            tb_net_request_refresh();
            update_hint();
            return;
        }
        if (event->btn == BSP_BTN_DOWN && s_page == 2 && s_have_data &&
            s_data.board_count > 1) {
            switch_board(1);
            return;
        }
        s_reset_armed = false;
        update_hint();
        return;
    }

    s_reset_armed = false;
    if (event->btn == BSP_BTN_OK) {
        switch_page(s_page + 1);
    } else if (event->btn == BSP_BTN_UP) {
        scroll_detail(-1);
    } else if (event->btn == BSP_BTN_DOWN) {
        scroll_detail(1);
    }
}

static void tick_cb(lv_timer_t *timer)
{
    (void)timer;
    s_tick++;
    key_event_t event;
    while (s_key_queue != NULL && xQueueReceive(s_key_queue, &event, 0) == pdTRUE) {
        handle_key(&event);
    }

    if (tb_portal_active() != s_portal_visible) {
        show_portal(tb_portal_active());
    }

    // Backlight-only power saving: the radio and the 5-minute fetch stay alive,
    // so a key wakes instantly without a reconnect or a provisioning round-trip.
    if (!s_portal_visible && !s_sleeping) {
        const int64_t idle = esp_timer_get_time() - s_last_activity_us;
        if (idle >= SLEEP_OFF_US) {
            s_sleeping = true;
            s_dimmed = false;
            bsp_display_backlight(0);
            tb_net_set_low_power(true);
            if (s_tick_timer != NULL) {
                lv_timer_set_period(s_tick_timer, 500);
            }
        } else if (!s_dimmed && idle >= SLEEP_DIM_US) {
            s_dimmed = true;
            bsp_display_backlight(25);
        }
    }

    if (tb_net_lock()) {
        if (tb_net_take_dirty()) {
            s_data = *tb_net_summary();
            s_have_data = tb_net_has_data();
            s_loading = false;
            if (!s_sleeping) {
                render_all();
            }
        }
        tb_net_unlock();
    }

    if (!s_sleeping) {
        if (s_tick % 10 == 0) {
            update_clock();
            update_network_indicator();
        }
        if (s_page == 2 && s_tick % 100 == 0) {
            update_hint();
        }
        if (s_tick % 300 == 0) {
            update_battery();
        }
        const tb_net_state_t state = tb_net_state();
        if (s_loading && (state == TB_NET_ONLINE || state == TB_NET_OFFLINE ||
                          state == TB_NET_WIFI_FAILED)) {
            s_loading = false;
            update_hint();
        }
    }
}

static void build_top_and_tabs(void)
{
    s_time_label = label_create(s_main, "--:--", &toolbox_font_14, COLOR_INK, 14, 4, 60, 18);
    s_net_dot = box_create(s_main, 130, 10, 6, 6, COLOR_DIM, 3, false);
    s_net_label = label_create(s_main, "启动中", &toolbox_font_14, COLOR_MUTED, 140, 4, 48, 18);
    s_battery_label = label_create(s_main, "—", &toolbox_font_14, COLOR_GREEN, 190, 4, 38, 18);
    lv_obj_set_style_text_align(s_battery_label, LV_TEXT_ALIGN_RIGHT, 0);

    static const char *TAB_TEXT[3] = {"站点", "到期", "榜单"};
    for (int i = 0; i < 3; i++) {
        s_tab_buttons[i] = box_create(s_main, 12 + i * 72, 30, 70, 28, 0x14263D, 9, true);
        s_tab_labels[i] = label_create(s_tab_buttons[i], TAB_TEXT[i], &toolbox_font_14, COLOR_MUTED,
                                       0, 4, 70, 20);
        lv_obj_set_style_text_align(s_tab_labels[i], LV_TEXT_ALIGN_CENTER, 0);
    }

    s_hint_label = label_create(s_main, "OK翻页 · ↑↓详情 · 长按OK刷新", &toolbox_font_12, COLOR_MUTED,
                                12, 294, 216, 18);
    lv_obj_set_style_text_align(s_hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(s_hint_label, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_hint_label, LV_OPA_COVER, 0);
}

static void build_site_page(lv_obj_t *parent)
{
    s_site_name = label_create(parent, "正在读取…", &toolbox_font_20, COLOR_INK, 12, 8, 196, 24);
    s_site_sub = label_create(parent, "今日浏览 · 全部", &toolbox_font_14, COLOR_MUTED, 12, 38, 196, 18);
    s_site_pv = label_create(parent, "—", &toolbox_font_28, COLOR_CYAN, 10, 54, 110, 36);
    s_site_visits = label_create(parent, "访问 —", &toolbox_font_14, COLOR_MUTED, 116, 70, 94, 18);
    lv_obj_set_style_text_align(s_site_visits, LV_TEXT_ALIGN_RIGHT, 0);
    box_create(parent, 12, 96, 196, 1, COLOR_LINE, 0, false);
    s_site_week = label_create(parent, "近 7 天", &toolbox_font_14, COLOR_MUTED, 12, 102, 120, 18);
    (void)s_site_week;

    for (int i = 0; i < 7; i++) {
        s_bars[i] = box_create(parent, 16 + i * 28, 170, 18, 3, i == 6 ? COLOR_VIOLET : COLOR_CYAN, 4,
                               false);
    }
    lv_obj_t *left = label_create(parent, "7 天前", &toolbox_font_14, COLOR_DIM, 12, 174, 60, 18);
    lv_obj_t *right = label_create(parent, "今天", &toolbox_font_14, COLOR_DIM, 160, 174, 48, 18);
    lv_obj_set_style_text_align(right, LV_TEXT_ALIGN_RIGHT, 0);
    (void)left;
    s_site_month = label_create(parent, "30 天 —", &toolbox_font_14, COLOR_MUTED, 12, 196, 196, 18);

}

static void build_expiry_page(lv_obj_t *parent)
{
    s_exp_title = label_create(parent, "到期倒计时", &toolbox_font_20, COLOR_INK, 12, 8, 196, 24);
    (void)s_exp_title;
    s_exp_sub = label_create(parent, "正在读取…", &toolbox_font_14, COLOR_MUTED, 12, 36, 196, 18);

    for (int i = 0; i < 5; i++) {
        const int y = 60 + i * 30;
        s_exp_rows[i] = box_create(parent, 0, y, 220, 28, COLOR_BG, 0, true);
        s_exp_bars[i] = box_create(s_exp_rows[i], 12, 3, 3, 20, COLOR_DIM, 2, false);
        s_exp_names[i] = label_create(s_exp_rows[i], "", &toolbox_font_12, COLOR_INK, 22, 4, 112, 18);
        s_exp_badges[i] = label_create(s_exp_rows[i], "", &toolbox_font_14, COLOR_MUTED, 138, 4, 74, 20);
        lv_obj_set_style_text_align(s_exp_badges[i], LV_TEXT_ALIGN_RIGHT, 0);
        if (i < 4) {
            box_create(s_exp_rows[i], 12, 27, 196, 1, COLOR_LINE, 0, false);
        }
    }
}

static void build_board_page(lv_obj_t *parent)
{
    s_board_title = label_create(parent, "模型榜单", &toolbox_font_20, COLOR_INK, 12, 8, 196, 24);
    s_board_meta = label_create(parent, "正在读取…", &toolbox_font_12, COLOR_MUTED, 12, 36, 196, 18);

    for (int i = 0; i < 5; i++) {
        const int y = 62 + i * 28;
        s_board_rows[i] = box_create(parent, 0, y, 220, 26, COLOR_BG, 0, true);
        s_board_ranks[i] = label_create(s_board_rows[i], "", &toolbox_font_14, COLOR_MUTED, 8, 3, 26, 20);
        s_board_models[i] = label_create(s_board_rows[i], "", &toolbox_font_12, COLOR_INK, 40, 4, 120, 18);
        s_board_scores[i] = label_create(s_board_rows[i], "", &toolbox_font_14, COLOR_INK, 170, 3, 44, 20);
        lv_obj_set_style_text_align(s_board_scores[i], LV_TEXT_ALIGN_RIGHT, 0);
        if (i < 4) {
            box_create(s_board_rows[i], 12, 25, 196, 1, COLOR_LINE, 0, false);
        }
    }
}

static void build_portal_page(lv_obj_t *screen)
{
    s_portal_page = box_create(screen, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG, 0, false);
    label_create(s_portal_page, "配网模式", &toolbox_font_20, COLOR_INK, 16, 8, 200, 24);
    label_create(s_portal_page, "手机连接设备 Wi-Fi 后扫码", &toolbox_font_12, COLOR_MUTED, 16, 36,
                 208, 18);

    s_portal_qr = lv_qrcode_create(s_portal_page);
    lv_qrcode_set_size(s_portal_qr, 132);
    lv_qrcode_set_dark_color(s_portal_qr, lv_color_hex(0x071018));
    lv_qrcode_set_light_color(s_portal_qr, lv_color_hex(0xFFFFFF));
    lv_qrcode_set_quiet_zone(s_portal_qr, true);
    lv_obj_set_pos(s_portal_qr, 54, 56);
    lv_obj_set_style_bg_color(s_portal_qr, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(s_portal_qr, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_portal_qr, 0, 0);
    lv_obj_set_style_pad_all(s_portal_qr, 0, 0);
    lv_qrcode_set_data(s_portal_qr, "http://192.168.4.1/");

    s_portal_ssid_label = label_create(s_portal_page, "AI-Toolbox", &toolbox_font_14, COLOR_CYAN, 16,
                                       192, 208, 18);
    label_create(s_portal_page, "扫码打开 192.168.4.1", &toolbox_font_12, COLOR_MUTED, 16, 214, 208,
                 18);
    label_create(s_portal_page, "配网页密码 toolbox123", &toolbox_font_12, COLOR_DIM, 16, 236, 208,
                 18);
    label_create(s_portal_page, "保存后自动重启", &toolbox_font_12, COLOR_DIM, 16, 262, 208, 18);
    lv_obj_add_flag(s_portal_page, LV_OBJ_FLAG_HIDDEN);
}

esp_err_t tb_ui_start(void)
{
    s_key_queue = xQueueCreate(8, sizeof(key_event_t));
    if (s_key_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    s_main = box_create(screen, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG, 0, true);
    build_top_and_tabs();

    lv_obj_t *card = box_create(s_main, CARD_X, CARD_Y, CARD_W, CARD_H, COLOR_CARD, 14, false);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(COLOR_LINE), 0);

    for (int i = 0; i < 3; i++) {
        s_pages[i] = box_create(card, 0, 0, CARD_W, CARD_H, COLOR_CARD, 0, false);
        lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    build_site_page(s_pages[0]);
    build_expiry_page(s_pages[1]);
    build_board_page(s_pages[2]);

    build_portal_page(screen);

    s_page = 0;
    s_board_index = 0;
    s_board_offset = 0;
    s_exp_offset = 0;
    s_site_index = 0;
    s_tick = 0;
    s_sleeping = false;
    s_dimmed = false;
    s_swallow_until_release = false;
    s_last_activity_us = esp_timer_get_time();
    s_portal_visible = false;
    update_tabs();
    update_clock();
    update_battery();
    update_network_indicator();
    update_hint();

    s_tick_timer = lv_timer_create(tick_cb, 100, NULL);
    return ESP_OK;
}

void tb_ui_key_event(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (s_key_queue == NULL) {
        return;
    }
    const key_event_t event = {.btn = btn, .ev = ev};
    (void)xQueueSend(s_key_queue, &event, 0);
}

void tb_ui_show_page(int page)
{
    if (page < 0 || page > 2 || !bsp_lvgl_lock(1000)) {
        return;
    }
    if (s_sleeping || s_dimmed) {
        wake_screen();
    }
    switch_page(page);
    bsp_lvgl_unlock();
}
