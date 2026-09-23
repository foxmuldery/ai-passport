#include "worry_seed_power_policy.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    worry_power_policy_t policy;
    worry_power_policy_init(&policy, 1000);

    assert(worry_power_policy_update(&policy, 30999, false) == WORRY_POWER_ACTIVE);
    assert(worry_power_policy_update(&policy, 31000, false) == WORRY_POWER_DIMMED);
    assert(worry_power_policy_update(&policy, 60999, false) == WORRY_POWER_DIMMED);
    assert(worry_power_policy_update(&policy, 61000, false) == WORRY_POWER_SCREEN_OFF);
    assert(worry_power_policy_wake_for_button(&policy, 70000));
    assert(policy.state == WORRY_POWER_ACTIVE);
    assert(!worry_power_policy_wake_for_button(&policy, 70001));

    worry_power_policy_init(&policy, 1000);
    assert(worry_power_policy_update(&policy, 300999, false) == WORRY_POWER_SCREEN_OFF);
    assert(worry_power_policy_update(&policy, 301000, false) == WORRY_POWER_DEEP_SLEEP);

    worry_power_policy_note_activity(&policy, 400000);
    assert(worry_power_policy_update(&policy, 400000, false) == WORRY_POWER_ACTIVE);

    assert(worry_power_policy_update(&policy, 800000, true) == WORRY_POWER_ACTIVE);
    assert(worry_power_policy_update(&policy, 800001, false) == WORRY_POWER_ACTIVE);
    assert(worry_power_policy_update(&policy, 830001, false) == WORRY_POWER_DIMMED);

    puts("Worry-seed power policy tests: PASS");
    return 0;
}
