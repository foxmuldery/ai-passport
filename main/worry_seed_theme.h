#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

typedef enum {
    WORRY_THEME_PIXEL = 0,
    WORRY_THEME_WATERCOLOR,
    WORRY_THEME_REALISTIC,
    WORRY_THEME_INK,
    WORRY_THEME_STARLIGHT,
    WORRY_THEME_COUNT,
} worry_theme_id_t;

typedef enum {
    WORRY_THEME_ICON_WATER = 0,
    WORRY_THEME_ICON_LEAF,
    WORRY_THEME_ICON_FRUIT,
    WORRY_THEME_ICON_MIC,
    WORRY_THEME_ICON_PLAY,
    WORRY_THEME_ICON_STOP,
    WORRY_THEME_ICON_BACK,
    WORRY_THEME_ICON_UP,
    WORRY_THEME_ICON_DOWN,
    WORRY_THEME_ICON_CONFIRM,
    WORRY_THEME_ICON_COUNT,
} worry_theme_icon_t;

#define WORRY_THEME_GROWTH_COUNT 5

typedef struct {
    worry_theme_id_t id;
    const char *label;
    uint32_t paper;
    uint32_t ink;
    uint32_t primary;
    uint32_t secondary;
    uint32_t soil;
    uint32_t accent;
    uint32_t footer;
    lv_opa_t hero_opa;
    const lv_image_dsc_t *hero;
    const lv_image_dsc_t *growth[WORRY_THEME_GROWTH_COUNT];
    const lv_image_dsc_t *icons[WORRY_THEME_ICON_COUNT];
} worry_theme_t;

void worry_seed_theme_init(void);
const worry_theme_t *worry_seed_theme_current(void);
bool worry_seed_theme_next(void);
bool worry_seed_theme_previous(void);
