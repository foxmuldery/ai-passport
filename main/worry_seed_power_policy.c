#include "worry_seed_power_policy.h"

#include <stddef.h>

void worry_power_policy_init(worry_power_policy_t *policy, uint64_t now_ms) {
    if (!policy) return;
    policy->last_activity_ms = now_ms;
    policy->state = WORRY_POWER_ACTIVE;
}

void worry_power_policy_note_activity(worry_power_policy_t *policy,
                                      uint64_t now_ms) {
    if (!policy) return;
    policy->last_activity_ms = now_ms;
    policy->state = WORRY_POWER_ACTIVE;
}

bool worry_power_policy_wake_for_button(worry_power_policy_t *policy,
                                        uint64_t now_ms) {
    if (!policy) return false;
    bool consume = policy->state == WORRY_POWER_SCREEN_OFF;
    worry_power_policy_note_activity(policy, now_ms);
    return consume;
}

worry_power_state_t worry_power_policy_update(worry_power_policy_t *policy,
                                               uint64_t now_ms, bool busy) {
    if (!policy) return WORRY_POWER_ACTIVE;
    if (busy || now_ms < policy->last_activity_ms) {
        worry_power_policy_note_activity(policy, now_ms);
        return policy->state;
    }

    uint64_t idle_ms = now_ms - policy->last_activity_ms;
    if (idle_ms >= WORRY_POWER_DEEP_SLEEP_AFTER_MS) {
        policy->state = WORRY_POWER_DEEP_SLEEP;
    } else if (idle_ms >= WORRY_POWER_SCREEN_OFF_AFTER_MS) {
        policy->state = WORRY_POWER_SCREEN_OFF;
    } else if (idle_ms >= WORRY_POWER_DIM_AFTER_MS) {
        policy->state = WORRY_POWER_DIMMED;
    } else {
        policy->state = WORRY_POWER_ACTIVE;
    }
    return policy->state;
}
