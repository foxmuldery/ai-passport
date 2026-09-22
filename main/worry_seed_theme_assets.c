#include "worry_seed_theme.h"

#define THEME_ASSET_BYTES 153600U
#define THEME_STRIDE 480U
#define STAGE_W 128U
#define STAGE_H 108U
#define STAGE_BYTES (STAGE_W * STAGE_H * 3U)
#define ICON_W 40U
#define ICON_H 40U
#define ICON_BYTES (ICON_W * ICON_H * 3U)

extern const uint8_t pixel_rgb565_start[] asm("_binary_pixel_rgb565_start");
extern const uint8_t watercolor_rgb565_start[] asm("_binary_watercolor_rgb565_start");
extern const uint8_t realistic_rgb565_start[] asm("_binary_realistic_rgb565_start");
extern const uint8_t ink_rgb565_start[] asm("_binary_ink_rgb565_start");
extern const uint8_t starlight_rgb565_start[] asm("_binary_starlight_rgb565_start");

#define THEME_IMAGE(name, ptr) \
    const lv_image_dsc_t worry_theme_##name##_image = { \
        .header = { \
            .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565, \
            .w = 240, .h = 320, .stride = THEME_STRIDE, \
        }, \
        .data_size = THEME_ASSET_BYTES, .data = ptr, \
    }

THEME_IMAGE(pixel, pixel_rgb565_start);
THEME_IMAGE(watercolor, watercolor_rgb565_start);
THEME_IMAGE(realistic, realistic_rgb565_start);
THEME_IMAGE(ink, ink_rgb565_start);
THEME_IMAGE(starlight, starlight_rgb565_start);

#define DECLARE_LAYER_SYMBOLS(theme) \
    extern const uint8_t theme##_stage_0_rgb565a8_start[] asm("_binary_" #theme "_stage_0_rgb565a8_start"); \
    extern const uint8_t theme##_stage_1_rgb565a8_start[] asm("_binary_" #theme "_stage_1_rgb565a8_start"); \
    extern const uint8_t theme##_stage_2_rgb565a8_start[] asm("_binary_" #theme "_stage_2_rgb565a8_start"); \
    extern const uint8_t theme##_stage_3_rgb565a8_start[] asm("_binary_" #theme "_stage_3_rgb565a8_start"); \
    extern const uint8_t theme##_stage_4_rgb565a8_start[] asm("_binary_" #theme "_stage_4_rgb565a8_start"); \
    extern const uint8_t theme##_icon_water_rgb565a8_start[] asm("_binary_" #theme "_icon_water_rgb565a8_start"); \
    extern const uint8_t theme##_icon_leaf_rgb565a8_start[] asm("_binary_" #theme "_icon_leaf_rgb565a8_start"); \
    extern const uint8_t theme##_icon_fruit_rgb565a8_start[] asm("_binary_" #theme "_icon_fruit_rgb565a8_start"); \
    extern const uint8_t theme##_icon_mic_rgb565a8_start[] asm("_binary_" #theme "_icon_mic_rgb565a8_start"); \
    extern const uint8_t theme##_icon_play_rgb565a8_start[] asm("_binary_" #theme "_icon_play_rgb565a8_start"); \
    extern const uint8_t theme##_icon_stop_rgb565a8_start[] asm("_binary_" #theme "_icon_stop_rgb565a8_start"); \
    extern const uint8_t theme##_icon_back_rgb565a8_start[] asm("_binary_" #theme "_icon_back_rgb565a8_start"); \
    extern const uint8_t theme##_icon_up_rgb565a8_start[] asm("_binary_" #theme "_icon_up_rgb565a8_start"); \
    extern const uint8_t theme##_icon_down_rgb565a8_start[] asm("_binary_" #theme "_icon_down_rgb565a8_start"); \
    extern const uint8_t theme##_icon_confirm_rgb565a8_start[] asm("_binary_" #theme "_icon_confirm_rgb565a8_start")

DECLARE_LAYER_SYMBOLS(pixel);
DECLARE_LAYER_SYMBOLS(watercolor);
DECLARE_LAYER_SYMBOLS(realistic);
DECLARE_LAYER_SYMBOLS(ink);
DECLARE_LAYER_SYMBOLS(starlight);

#define LAYER_IMAGE(theme, kind, asset_w, asset_h, asset_bytes, asset_stride) \
    const lv_image_dsc_t worry_theme_##theme##_##kind = { \
        .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565A8, \
                    .w = asset_w, .h = asset_h, .stride = asset_stride }, \
        .data_size = asset_bytes, .data = theme##_##kind##_rgb565a8_start, \
    }

#define THEME_LAYERS(theme) \
    LAYER_IMAGE(theme, stage_0, STAGE_W, STAGE_H, STAGE_BYTES, STAGE_W * 2U); \
    LAYER_IMAGE(theme, stage_1, STAGE_W, STAGE_H, STAGE_BYTES, STAGE_W * 2U); \
    LAYER_IMAGE(theme, stage_2, STAGE_W, STAGE_H, STAGE_BYTES, STAGE_W * 2U); \
    LAYER_IMAGE(theme, stage_3, STAGE_W, STAGE_H, STAGE_BYTES, STAGE_W * 2U); \
    LAYER_IMAGE(theme, stage_4, STAGE_W, STAGE_H, STAGE_BYTES, STAGE_W * 2U); \
    LAYER_IMAGE(theme, icon_water, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_leaf, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_fruit, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_mic, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_play, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_stop, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_back, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_up, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_down, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U); \
    LAYER_IMAGE(theme, icon_confirm, ICON_W, ICON_H, ICON_BYTES, ICON_W * 2U)

THEME_LAYERS(pixel);
THEME_LAYERS(watercolor);
THEME_LAYERS(realistic);
THEME_LAYERS(ink);
THEME_LAYERS(starlight);
