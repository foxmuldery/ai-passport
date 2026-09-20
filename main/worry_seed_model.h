#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WORRY_SEED_MODEL_VERSION 1U
#define WORRY_SEED_MAX_SEEDS 12U
#define WORRY_SEED_MAX_UPDATES 5U
#define WORRY_SEED_MAX_RECORDINGS (WORRY_SEED_MAX_SEEDS * (1U + WORRY_SEED_MAX_UPDATES))

typedef enum {
    WORRY_CATEGORY_WORK = 0,
    WORRY_CATEGORY_RELATIONSHIP,
    WORRY_CATEGORY_FAMILY,
    WORRY_CATEGORY_HEALTH,
    WORRY_CATEGORY_OTHER,
    WORRY_CATEGORY_COUNT,
} worry_category_t;

typedef enum {
    WORRY_SEED_EMPTY = 0,
    WORRY_SEED_GROWING,
    WORRY_SEED_FRUITED,
} worry_seed_status_t;

typedef enum {
    WORRY_GROWTH_SEED = 0,
    WORRY_GROWTH_SPROUT,
    WORRY_GROWTH_SAPLING,
    WORRY_GROWTH_TREE,
    WORRY_GROWTH_FRUIT,
} worry_growth_stage_t;

typedef struct {
    uint16_t recording_id;
    uint16_t duration_sec;
} worry_recording_ref_t;

typedef struct {
    uint16_t id;
    uint8_t status;
    uint8_t category;
    uint8_t update_count;
    uint8_t reserved;
    worry_recording_ref_t origin;
    worry_recording_ref_t updates[WORRY_SEED_MAX_UPDATES];
} worry_seed_t;

typedef struct {
    uint16_t version;
    uint16_t next_seed_id;
    uint16_t next_recording_id;
    uint8_t count;
    uint8_t selected;
    worry_seed_t seeds[WORRY_SEED_MAX_SEEDS];
} worry_seed_model_t;

void worry_seed_model_defaults(worry_seed_model_t *model);
bool worry_seed_model_validate(worry_seed_model_t *model);
int worry_seed_model_add(worry_seed_model_t *model, worry_category_t category,
                         uint16_t recording_id, uint16_t duration_sec);
bool worry_seed_model_add_update(worry_seed_model_t *model, size_t index,
                                 uint16_t recording_id, uint16_t duration_sec);
bool worry_seed_model_mark_fruited(worry_seed_model_t *model, size_t index);
bool worry_seed_model_remove(worry_seed_model_t *model, size_t index);
bool worry_seed_model_select(worry_seed_model_t *model, int direction);
worry_growth_stage_t worry_seed_model_stage(const worry_seed_t *seed);
uint16_t worry_seed_model_allocate_recording_id(worry_seed_model_t *model);
const char *worry_seed_category_label(worry_category_t category);
const char *worry_seed_stage_label(worry_growth_stage_t stage);
