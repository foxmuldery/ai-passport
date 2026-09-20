#pragma once

#include "worry_seed_model.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

bool worry_seed_store_init(worry_seed_model_t *model);
bool worry_seed_store_save(const worry_seed_model_t *model);
bool worry_seed_store_audio_path(uint16_t recording_id, bool temporary,
                                 char *path, size_t path_size);
FILE *worry_seed_store_audio_open(uint16_t recording_id, const char *mode);
bool worry_seed_store_audio_commit(uint16_t recording_id);
bool worry_seed_store_audio_remove(uint16_t recording_id);
size_t worry_seed_store_free_bytes(void);
