#include "worry_seed_model.h"

#include <string.h>

void worry_seed_model_defaults(worry_seed_model_t *model) {
    if (!model) return;
    memset(model, 0, sizeof(*model));
    model->version = WORRY_SEED_MODEL_VERSION;
    model->next_seed_id = 1;
    model->next_recording_id = 1;
}

bool worry_seed_model_validate(worry_seed_model_t *model) {
    if (!model || model->version != WORRY_SEED_MODEL_VERSION ||
        model->count > WORRY_SEED_MAX_SEEDS ||
        (model->count && model->selected >= model->count)) {
        return false;
    }
    for (size_t i = 0; i < model->count; i++) {
        worry_seed_t *seed = &model->seeds[i];
        if (seed->id == 0 || seed->status < WORRY_SEED_GROWING ||
            seed->status > WORRY_SEED_FRUITED ||
            seed->category >= WORRY_CATEGORY_COUNT ||
            seed->origin.recording_id == 0 ||
            seed->update_count > WORRY_SEED_MAX_UPDATES) return false;
    }
    if (model->next_seed_id == 0) model->next_seed_id = 1;
    if (model->next_recording_id == 0) model->next_recording_id = 1;
    return true;
}

uint16_t worry_seed_model_allocate_recording_id(worry_seed_model_t *model) {
    if (!model) return 0;
    uint16_t id = model->next_recording_id++;
    if (model->next_recording_id == 0) model->next_recording_id = 1;
    return id;
}

int worry_seed_model_add(worry_seed_model_t *model, worry_category_t category,
                         uint16_t recording_id, uint16_t duration_sec) {
    if (!model || model->count >= WORRY_SEED_MAX_SEEDS ||
        category >= WORRY_CATEGORY_COUNT || recording_id == 0 || duration_sec == 0) {
        return -1;
    }
    size_t index = model->count++;
    worry_seed_t *seed = &model->seeds[index];
    memset(seed, 0, sizeof(*seed));
    seed->id = model->next_seed_id++;
    if (model->next_seed_id == 0) model->next_seed_id = 1;
    seed->status = WORRY_SEED_GROWING;
    seed->category = (uint8_t)category;
    seed->origin.recording_id = recording_id;
    seed->origin.duration_sec = duration_sec;
    model->selected = (uint8_t)index;
    return (int)index;
}

bool worry_seed_model_add_update(worry_seed_model_t *model, size_t index,
                                 uint16_t recording_id, uint16_t duration_sec) {
    if (!model || index >= model->count || recording_id == 0 || duration_sec == 0) return false;
    worry_seed_t *seed = &model->seeds[index];
    if (seed->status != WORRY_SEED_GROWING || seed->update_count >= WORRY_SEED_MAX_UPDATES) {
        return false;
    }
    seed->updates[seed->update_count++] = (worry_recording_ref_t) {
        .recording_id = recording_id,
        .duration_sec = duration_sec,
    };
    return true;
}

bool worry_seed_model_mark_fruited(worry_seed_model_t *model, size_t index) {
    if (!model || index >= model->count || model->seeds[index].status != WORRY_SEED_GROWING) {
        return false;
    }
    model->seeds[index].status = WORRY_SEED_FRUITED;
    return true;
}

bool worry_seed_model_remove(worry_seed_model_t *model, size_t index) {
    if (!model || index >= model->count) return false;
    if (index + 1 < model->count) {
        memmove(&model->seeds[index], &model->seeds[index + 1],
                (model->count - index - 1U) * sizeof(model->seeds[0]));
    }
    memset(&model->seeds[model->count - 1U], 0, sizeof(model->seeds[0]));
    model->count--;
    if (model->count == 0) model->selected = 0;
    else if (model->selected >= model->count) model->selected = model->count - 1U;
    return true;
}

bool worry_seed_model_select(worry_seed_model_t *model, int direction) {
    if (!model || model->count == 0 || direction == 0) return false;
    int next = (int)model->selected + (direction > 0 ? 1 : -1);
    if (next < 0) next = model->count - 1;
    if (next >= model->count) next = 0;
    model->selected = (uint8_t)next;
    return true;
}

worry_growth_stage_t worry_seed_model_stage(const worry_seed_t *seed) {
    if (!seed || seed->status == WORRY_SEED_EMPTY) return WORRY_GROWTH_SEED;
    if (seed->status == WORRY_SEED_FRUITED) return WORRY_GROWTH_FRUIT;
    if (seed->update_count == 0) return WORRY_GROWTH_SEED;
    if (seed->update_count == 1) return WORRY_GROWTH_SPROUT;
    if (seed->update_count <= 3) return WORRY_GROWTH_SAPLING;
    return WORRY_GROWTH_TREE;
}

const char *worry_seed_category_label(worry_category_t category) {
    static const char *const labels[] = { "工作", "关系", "家庭", "健康", "其他" };
    return category < WORRY_CATEGORY_COUNT ? labels[category] : "其他";
}

const char *worry_seed_stage_label(worry_growth_stage_t stage) {
    static const char *const labels[] = { "种子", "发芽", "幼苗", "成树", "结果" };
    return stage <= WORRY_GROWTH_FRUIT ? labels[stage] : "种子";
}
