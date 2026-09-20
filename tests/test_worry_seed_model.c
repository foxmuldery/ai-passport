#include "worry_seed_model.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    worry_seed_model_t model;
    worry_seed_model_defaults(&model);
    assert(worry_seed_model_validate(&model));
    assert(model.count == 0);

    uint16_t origin = worry_seed_model_allocate_recording_id(&model);
    assert(origin == 1);
    assert(worry_seed_model_add(&model, WORRY_CATEGORY_WORK, origin, 60) == 0);
    assert(model.count == 1 && model.selected == 0);
    assert(worry_seed_model_stage(&model.seeds[0]) == WORRY_GROWTH_SEED);

    for (unsigned i = 0; i < WORRY_SEED_MAX_UPDATES; i++) {
        uint16_t id = worry_seed_model_allocate_recording_id(&model);
        assert(worry_seed_model_add_update(&model, 0, id, 30));
    }
    assert(!worry_seed_model_add_update(&model, 0, 99, 30));
    assert(worry_seed_model_stage(&model.seeds[0]) == WORRY_GROWTH_TREE);
    assert(worry_seed_model_mark_fruited(&model, 0));
    assert(worry_seed_model_stage(&model.seeds[0]) == WORRY_GROWTH_FRUIT);
    assert(!worry_seed_model_add_update(&model, 0, 100, 30));

    assert(worry_seed_model_add(&model, WORRY_CATEGORY_FAMILY, 88, 12) == 1);
    assert(worry_seed_model_select(&model, 1));
    assert(model.selected == 0);
    assert(worry_seed_model_select(&model, -1));
    assert(model.selected == 1);
    assert(worry_seed_model_remove(&model, 0));
    assert(model.count == 1 && model.seeds[0].category == WORRY_CATEGORY_FAMILY);
    assert(worry_seed_model_validate(&model));

    model.version = 99;
    assert(!worry_seed_model_validate(&model));
    puts("worry_seed_model: PASS");
    return 0;
}
