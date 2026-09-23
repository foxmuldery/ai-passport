#pragma once

#include <stdbool.h>
#include <stdint.h>

#define WORRY_POWER_DIM_AFTER_MS       30000ULL
#define WORRY_POWER_SCREEN_OFF_AFTER_MS 60000ULL
#define WORRY_POWER_DEEP_SLEEP_AFTER_MS 300000ULL
#define WORRY_POWER_ACTIVE_BRIGHTNESS  82U
#define WORRY_POWER_DIM_BRIGHTNESS     18U

typedef enum {
    WORRY_POWER_ACTIVE = 0,
    WORRY_POWER_DIMMED,
    WORRY_POWER_SCREEN_OFF,
    WORRY_POWER_DEEP_SLEEP,
} worry_power_state_t;

typedef struct {
    uint64_t last_activity_ms;
    worry_power_state_t state;
} worry_power_policy_t;

void worry_power_policy_init(worry_power_policy_t *policy, uint64_t now_ms);
void worry_power_policy_note_activity(worry_power_policy_t *policy, uint64_t now_ms);

// Returns true when the button must be consumed solely to wake a black screen.
// A button pressed while merely dimmed restores full brightness and may still
// perform its normal application action.
bool worry_power_policy_wake_for_button(worry_power_policy_t *policy,
                                        uint64_t now_ms);

// Busy audio/recording holds the application active and restarts the idle
// interval, so it can never be interrupted by automatic sleep.
worry_power_state_t worry_power_policy_update(worry_power_policy_t *policy,
                                               uint64_t now_ms, bool busy);
