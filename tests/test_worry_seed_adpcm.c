#include "worry_seed_adpcm.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int16_t input[800];
    for (size_t i = 0; i < 800; i++) {
        int phase = (int)(i % 80);
        input[i] = (int16_t)((phase < 40 ? phase : 80 - phase) * 500 - 10000);
    }
    uint8_t encoded[400];
    int16_t decoded[800];
    worry_adpcm_state_t enc, dec;
    worry_adpcm_reset(&enc);
    worry_adpcm_reset(&dec);
    assert(worry_adpcm_encode(&enc, input, 800, encoded) == 400);
    assert(worry_adpcm_decode(&dec, encoded, 400, decoded, 800) == 800);
    long long absolute_error = 0;
    for (size_t i = 0; i < 800; i++) absolute_error += llabs((long long)input[i] - decoded[i]);
    assert(absolute_error / 800 < 1800);
    puts("worry_seed_adpcm: PASS");
    return 0;
}
