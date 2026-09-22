#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define WORRY_AUDIO_SAMPLE_RATE 8000U
#define WORRY_AUDIO_MAX_SECONDS 60U
#define WORRY_AUDIO_MIN_SECONDS 1U
#define WORRY_AUDIO_MAX_FILE_BYTES (WORRY_AUDIO_SAMPLE_RATE * WORRY_AUDIO_MAX_SECONDS / 2U + 32U)

typedef enum {
    WORRY_AUDIO_EVENT_RECORDING_STARTED = 0,
    WORRY_AUDIO_EVENT_RECORDING_PROGRESS,
    WORRY_AUDIO_EVENT_RECORDING_FINISHED,
    WORRY_AUDIO_EVENT_PLAYBACK_STARTED,
    WORRY_AUDIO_EVENT_PLAYBACK_FINISHED,
    WORRY_AUDIO_EVENT_PLAYBACK_STOPPED,
    WORRY_AUDIO_EVENT_ERROR,
} worry_audio_event_t;

typedef enum {
    WORRY_AUDIO_TONE_BUTTON = 0,
    WORRY_AUDIO_TONE_PLANT,
    WORRY_AUDIO_TONE_VOICE,
    WORRY_AUDIO_TONE_FRUIT,
} worry_audio_tone_t;

typedef void (*worry_audio_callback_t)(worry_audio_event_t event, uint16_t recording_id,
                                       uint16_t seconds, void *user);

esp_err_t worry_seed_audio_init(worry_audio_callback_t callback, void *user);
bool worry_seed_audio_record(uint16_t recording_id);
bool worry_seed_audio_play(uint16_t recording_id);
bool worry_seed_audio_stop(void);
// Queue a short, non-blocking key acknowledgement tone.
bool worry_seed_audio_beep(void);
// Queue a non-blocking transition tone for the product flow.
bool worry_seed_audio_tone(worry_audio_tone_t tone);
bool worry_seed_audio_busy(void);
