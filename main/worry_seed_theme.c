#include "worry_seed_theme.h"

#include "nvs.h"

#define THEME_NAMESPACE "worrytheme"
#define THEME_KEY "id"

extern const lv_image_dsc_t worry_theme_pixel_image;
extern const lv_image_dsc_t worry_theme_watercolor_image;
extern const lv_image_dsc_t worry_theme_realistic_image;
extern const lv_image_dsc_t worry_theme_ink_image;
extern const lv_image_dsc_t worry_theme_starlight_image;

#define DECLARE_THEME_LAYERS(name) \
    extern const lv_image_dsc_t worry_theme_##name##_stage_0; \
    extern const lv_image_dsc_t worry_theme_##name##_stage_1; \
    extern const lv_image_dsc_t worry_theme_##name##_stage_2; \
    extern const lv_image_dsc_t worry_theme_##name##_stage_3; \
    extern const lv_image_dsc_t worry_theme_##name##_stage_4; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_water; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_leaf; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_fruit; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_mic; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_play; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_stop; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_back; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_up; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_down; \
    extern const lv_image_dsc_t worry_theme_##name##_icon_confirm

DECLARE_THEME_LAYERS(pixel);
DECLARE_THEME_LAYERS(watercolor);
DECLARE_THEME_LAYERS(realistic);
DECLARE_THEME_LAYERS(ink);
DECLARE_THEME_LAYERS(starlight);

#define GROWTH_ASSETS(name) { \
    &worry_theme_##name##_stage_0, &worry_theme_##name##_stage_1, \
    &worry_theme_##name##_stage_2, &worry_theme_##name##_stage_3, \
    &worry_theme_##name##_stage_4 }
#define ICON_ASSETS(name) { \
    &worry_theme_##name##_icon_water, &worry_theme_##name##_icon_leaf, \
    &worry_theme_##name##_icon_fruit, &worry_theme_##name##_icon_mic, \
    &worry_theme_##name##_icon_play, &worry_theme_##name##_icon_stop, \
    &worry_theme_##name##_icon_back, &worry_theme_##name##_icon_up, \
    &worry_theme_##name##_icon_down, &worry_theme_##name##_icon_confirm }

static worry_theme_id_t s_current = WORRY_THEME_PIXEL;
static nvs_handle_t s_nvs;
static bool s_ready;

static const worry_theme_t s_themes[WORRY_THEME_COUNT] = {
    [WORRY_THEME_PIXEL] = {
        .id = WORRY_THEME_PIXEL, .label = "", .paper = 0x1D2A27,
        .ink = 0xF6F1DE, .primary = 0x82A86B, .secondary = 0x42634E,
        .soil = 0x9B7048, .accent = 0xE77B68, .footer = 0x24443A,
        .hero_opa = LV_OPA_COVER, .hero = &worry_theme_pixel_image,
        .growth = GROWTH_ASSETS(pixel), .icons = ICON_ASSETS(pixel),
    },
    [WORRY_THEME_WATERCOLOR] = {
        .id = WORRY_THEME_WATERCOLOR, .label = "浪漫水彩", .paper = 0xFFF8E8,
        .ink = 0x5A4630, .primary = 0x7A9E70, .secondary = 0xDDE8D2,
        .soil = 0xB9855F, .accent = 0xD98989, .footer = 0x6D8C68,
        .hero_opa = LV_OPA_COVER, .hero = &worry_theme_watercolor_image,
        .growth = GROWTH_ASSETS(watercolor), .icons = ICON_ASSETS(watercolor),
    },
    [WORRY_THEME_REALISTIC] = {
        .id = WORRY_THEME_REALISTIC, .label = "微缩写实", .paper = 0x2C2418,
        .ink = 0xFFF6DB, .primary = 0xC8D58F, .secondary = 0x6D8255,
        .soil = 0x765338, .accent = 0xF0A85C, .footer = 0x463728,
        .hero_opa = LV_OPA_COVER, .hero = &worry_theme_realistic_image,
        .growth = GROWTH_ASSETS(realistic), .icons = ICON_ASSETS(realistic),
    },
    [WORRY_THEME_INK] = {
        .id = WORRY_THEME_INK, .label = "东方水墨", .paper = 0xF3EBD4,
        .ink = 0x26382E, .primary = 0x66846B, .secondary = 0xD9D4B9,
        .soil = 0xA7774D, .accent = 0xD77A61, .footer = 0x506652,
        .hero_opa = LV_OPA_COVER, .hero = &worry_theme_ink_image,
        .growth = GROWTH_ASSETS(ink), .icons = ICON_ASSETS(ink),
    },
    [WORRY_THEME_STARLIGHT] = {
        .id = WORRY_THEME_STARLIGHT, .label = "星空梦境", .paper = 0x07143A,
        .ink = 0xFFF2C5, .primary = 0x8AA9FF, .secondary = 0x253E84,
        .soil = 0x51406E, .accent = 0xFFD36D, .footer = 0x111F59,
        .hero_opa = LV_OPA_COVER, .hero = &worry_theme_starlight_image,
        .growth = GROWTH_ASSETS(starlight), .icons = ICON_ASSETS(starlight),
    },
};

void worry_seed_theme_init(void) {
    if (s_ready) return;
    // Keep one stable art direction while the first theme is being refined.
    // Older NVS theme selections must not make the UI switch unexpectedly.
    s_current = WORRY_THEME_PIXEL;
    (void)nvs_open(THEME_NAMESPACE, NVS_READWRITE, &s_nvs);
    s_ready = true;
}

const worry_theme_t *worry_seed_theme_current(void) {
    return &s_themes[s_current];
}

static bool set_theme(worry_theme_id_t id) {
    if (id != WORRY_THEME_PIXEL) return false;
    s_current = id;
    if (s_nvs) {
        if (nvs_set_u8(s_nvs, THEME_KEY, (uint8_t)id) != ESP_OK || nvs_commit(s_nvs) != ESP_OK)
            return false;
    }
    return true;
}

bool worry_seed_theme_next(void) {
    return set_theme((worry_theme_id_t)((s_current + 1U) % WORRY_THEME_COUNT));
}

bool worry_seed_theme_previous(void) {
    return set_theme((worry_theme_id_t)((s_current + WORRY_THEME_COUNT - 1U) % WORRY_THEME_COUNT));
}
