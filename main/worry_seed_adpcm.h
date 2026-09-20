#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int16_t predictor;
    int8_t step_index;
} worry_adpcm_state_t;

void worry_adpcm_reset(worry_adpcm_state_t *state);
uint8_t worry_adpcm_encode_sample(worry_adpcm_state_t *state, int16_t sample);
int16_t worry_adpcm_decode_nibble(worry_adpcm_state_t *state, uint8_t nibble);
size_t worry_adpcm_encode(worry_adpcm_state_t *state, const int16_t *pcm,
                          size_t sample_count, uint8_t *output);
size_t worry_adpcm_decode(worry_adpcm_state_t *state, const uint8_t *input,
                          size_t byte_count, int16_t *pcm, size_t pcm_capacity);
