#include "worry_seed_adpcm.h"

#include <limits.h>

static const int16_t STEP_TABLE[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,
    73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,
    449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,
    2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,
    7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,
    24623,27086,29794,32767
};
static const int8_t INDEX_TABLE[16] = {
    -1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8
};

static int clamp_int(int value, int low, int high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

void worry_adpcm_reset(worry_adpcm_state_t *state) {
    if (!state) return;
    state->predictor = 0;
    state->step_index = 0;
}

uint8_t worry_adpcm_encode_sample(worry_adpcm_state_t *state, int16_t sample) {
    if (!state) return 0;
    int step = STEP_TABLE[(uint8_t)state->step_index];
    int diff = (int)sample - state->predictor;
    uint8_t code = 0;
    if (diff < 0) { code = 8; diff = -diff; }
    int delta = step >> 3;
    if (diff >= step) { code |= 4; diff -= step; delta += step; }
    if (diff >= (step >> 1)) { code |= 2; diff -= step >> 1; delta += step >> 1; }
    if (diff >= (step >> 2)) { code |= 1; delta += step >> 2; }
    int predictor = state->predictor + ((code & 8) ? -delta : delta);
    state->predictor = (int16_t)clamp_int(predictor, INT16_MIN, INT16_MAX);
    state->step_index = (int8_t)clamp_int(state->step_index + INDEX_TABLE[code], 0, 88);
    return code;
}

int16_t worry_adpcm_decode_nibble(worry_adpcm_state_t *state, uint8_t nibble) {
    if (!state) return 0;
    nibble &= 0x0f;
    int step = STEP_TABLE[(uint8_t)state->step_index];
    int delta = step >> 3;
    if (nibble & 4) delta += step;
    if (nibble & 2) delta += step >> 1;
    if (nibble & 1) delta += step >> 2;
    int predictor = state->predictor + ((nibble & 8) ? -delta : delta);
    state->predictor = (int16_t)clamp_int(predictor, INT16_MIN, INT16_MAX);
    state->step_index = (int8_t)clamp_int(state->step_index + INDEX_TABLE[nibble], 0, 88);
    return state->predictor;
}

size_t worry_adpcm_encode(worry_adpcm_state_t *state, const int16_t *pcm,
                          size_t sample_count, uint8_t *output) {
    if (!state || !pcm || !output) return 0;
    size_t out = 0;
    for (size_t i = 0; i < sample_count; i += 2) {
        uint8_t low = worry_adpcm_encode_sample(state, pcm[i]);
        uint8_t high = i + 1 < sample_count ? worry_adpcm_encode_sample(state, pcm[i + 1]) : 0;
        output[out++] = low | (uint8_t)(high << 4);
    }
    return out;
}

size_t worry_adpcm_decode(worry_adpcm_state_t *state, const uint8_t *input,
                          size_t byte_count, int16_t *pcm, size_t pcm_capacity) {
    if (!state || !input || !pcm) return 0;
    size_t out = 0;
    for (size_t i = 0; i < byte_count && out < pcm_capacity; i++) {
        pcm[out++] = worry_adpcm_decode_nibble(state, input[i] & 0x0f);
        if (out < pcm_capacity) pcm[out++] = worry_adpcm_decode_nibble(state, input[i] >> 4);
    }
    return out;
}
