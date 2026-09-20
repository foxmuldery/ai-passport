#include "worry_seed_audio.h"

#include <stdio.h>
#include <string.h>

#include "bsp_audio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "worry_seed_adpcm.h"
#include "worry_seed_store.h"

#define AUDIO_MAGIC 0x44415357U
#define AUDIO_VERSION 1U
#define AUDIO_FRAME_SAMPLES 256U
#define AUDIO_QUEUE_DEPTH 4U

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t sample_rate;
    uint32_t sample_count;
    uint32_t reserved;
} audio_header_t;

typedef enum { AUDIO_CMD_RECORD, AUDIO_CMD_PLAY, AUDIO_CMD_STOP } audio_command_type_t;
typedef struct { audio_command_type_t type; uint16_t recording_id; } audio_command_t;

static const char *TAG = "worry_audio";
static QueueHandle_t s_queue;
static TaskHandle_t s_task;
static worry_audio_callback_t s_callback;
static void *s_user;
static volatile bool s_busy;
static volatile bool s_stop;

static void notify(worry_audio_event_t event, uint16_t id, uint16_t seconds) {
    if (s_callback) s_callback(event, id, seconds, s_user);
}

static void check_stop_commands(void) {
    audio_command_t command;
    while (xQueueReceive(s_queue, &command, 0) == pdTRUE) {
        if (command.type == AUDIO_CMD_STOP) s_stop = true;
    }
}

static void record_file(uint16_t id) {
    if (worry_seed_store_free_bytes() < WORRY_AUDIO_MAX_FILE_BYTES + 4096U) {
        ESP_LOGW(TAG, "not enough space for a 60 second recording");
        notify(WORRY_AUDIO_EVENT_ERROR, id, 0);
        return;
    }
    if (bsp_audio_set_format(WORRY_AUDIO_SAMPLE_RATE, 16, 1) != ESP_OK) {
        notify(WORRY_AUDIO_EVENT_ERROR, id, 0);
        return;
    }
    FILE *file = worry_seed_store_audio_open(id, "wb+");
    if (!file) { notify(WORRY_AUDIO_EVENT_ERROR, id, 0); return; }
    audio_header_t header = {
        .magic = AUDIO_MAGIC,
        .version = AUDIO_VERSION,
        .sample_rate = WORRY_AUDIO_SAMPLE_RATE,
    };
    if (fwrite(&header, 1, sizeof(header), file) != sizeof(header)) {
        fclose(file); worry_seed_store_audio_remove(id); notify(WORRY_AUDIO_EVENT_ERROR, id, 0); return;
    }

    int16_t pcm[AUDIO_FRAME_SAMPLES];
    uint8_t encoded[AUDIO_FRAME_SAMPLES / 2U];
    worry_adpcm_state_t state;
    worry_adpcm_reset(&state);
    uint32_t max_samples = WORRY_AUDIO_SAMPLE_RATE * WORRY_AUDIO_MAX_SECONDS;
    uint16_t last_second = 0;
    s_stop = false;
    notify(WORRY_AUDIO_EVENT_RECORDING_STARTED, id, 0);

    while (!s_stop && header.sample_count < max_samples) {
        if (bsp_audio_read(pcm, sizeof(pcm)) != ESP_OK) break;
        size_t samples = AUDIO_FRAME_SAMPLES;
        if (samples > max_samples - header.sample_count) samples = max_samples - header.sample_count;
        size_t bytes = worry_adpcm_encode(&state, pcm, samples, encoded);
        if (fwrite(encoded, 1, bytes, file) != bytes) break;
        header.sample_count += samples;
        uint16_t seconds = header.sample_count / WORRY_AUDIO_SAMPLE_RATE;
        if (seconds != last_second) {
            last_second = seconds;
            notify(WORRY_AUDIO_EVENT_RECORDING_PROGRESS, id, seconds);
        }
        check_stop_commands();
    }

    bool ok = header.sample_count >= WORRY_AUDIO_SAMPLE_RATE * WORRY_AUDIO_MIN_SECONDS;
    if (ok && fseek(file, 0, SEEK_SET) == 0) {
        ok = fwrite(&header, 1, sizeof(header), file) == sizeof(header);
    }
    ok = fclose(file) == 0 && ok;
    if (ok) ok = worry_seed_store_audio_commit(id);
    if (!ok) worry_seed_store_audio_remove(id);
    uint16_t seconds = (uint16_t)((header.sample_count + WORRY_AUDIO_SAMPLE_RATE - 1U) /
                                  WORRY_AUDIO_SAMPLE_RATE);
    notify(ok ? WORRY_AUDIO_EVENT_RECORDING_FINISHED : WORRY_AUDIO_EVENT_ERROR, id, seconds);
}

static void play_file(uint16_t id) {
    if (bsp_audio_set_format(WORRY_AUDIO_SAMPLE_RATE, 16, 1) != ESP_OK) {
        notify(WORRY_AUDIO_EVENT_ERROR, id, 0); return;
    }
    FILE *file = worry_seed_store_audio_open(id, "rb");
    if (!file) { notify(WORRY_AUDIO_EVENT_ERROR, id, 0); return; }
    audio_header_t header;
    if (fread(&header, 1, sizeof(header), file) != sizeof(header) ||
        header.magic != AUDIO_MAGIC || header.version != AUDIO_VERSION ||
        header.sample_rate != WORRY_AUDIO_SAMPLE_RATE || header.sample_count == 0 ||
        header.sample_count > WORRY_AUDIO_SAMPLE_RATE * WORRY_AUDIO_MAX_SECONDS) {
        fclose(file); notify(WORRY_AUDIO_EVENT_ERROR, id, 0); return;
    }
    uint8_t encoded[AUDIO_FRAME_SAMPLES / 2U];
    int16_t pcm[AUDIO_FRAME_SAMPLES];
    worry_adpcm_state_t state;
    worry_adpcm_reset(&state);
    uint32_t remaining = header.sample_count;
    s_stop = false;
    notify(WORRY_AUDIO_EVENT_PLAYBACK_STARTED, id, 0);
    while (!s_stop && remaining) {
        size_t wanted_samples = remaining > AUDIO_FRAME_SAMPLES ? AUDIO_FRAME_SAMPLES : remaining;
        size_t wanted_bytes = (wanted_samples + 1U) / 2U;
        size_t bytes = fread(encoded, 1, wanted_bytes, file);
        if (bytes != wanted_bytes) break;
        size_t samples = worry_adpcm_decode(&state, encoded, bytes, pcm, wanted_samples);
        if (bsp_audio_write(pcm, samples * sizeof(pcm[0])) != ESP_OK) break;
        remaining -= samples;
        check_stop_commands();
    }
    fclose(file);
    notify(WORRY_AUDIO_EVENT_PLAYBACK_FINISHED, id,
           (uint16_t)(header.sample_count / WORRY_AUDIO_SAMPLE_RATE));
}

static void audio_task(void *arg) {
    (void)arg;
    audio_command_t command;
    for (;;) {
        if (xQueueReceive(s_queue, &command, portMAX_DELAY) != pdTRUE) continue;
        if (command.type == AUDIO_CMD_STOP) continue;
        s_busy = true;
        if (command.type == AUDIO_CMD_RECORD) record_file(command.recording_id);
        else if (command.type == AUDIO_CMD_PLAY) play_file(command.recording_id);
        s_busy = false;
    }
}

esp_err_t worry_seed_audio_init(worry_audio_callback_t callback, void *user) {
    if (s_task) { s_callback = callback; s_user = user; return ESP_OK; }
    s_callback = callback;
    s_user = user;
    s_queue = xQueueCreate(AUDIO_QUEUE_DEPTH, sizeof(audio_command_t));
    if (!s_queue) return ESP_ERR_NO_MEM;
    if (xTaskCreate(audio_task, "worry_audio", 4096, NULL, 4, &s_task) != pdPASS) {
        vQueueDelete(s_queue); s_queue = NULL; return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static bool send(audio_command_type_t type, uint16_t id) {
    if (!s_queue || (type != AUDIO_CMD_STOP && s_busy)) return false;
    audio_command_t command = { .type = type, .recording_id = id };
    if (type != AUDIO_CMD_STOP) s_busy = true;
    if (xQueueSend(s_queue, &command, 0) == pdTRUE) return true;
    if (type != AUDIO_CMD_STOP) s_busy = false;
    return false;
}

bool worry_seed_audio_record(uint16_t recording_id) { return recording_id && send(AUDIO_CMD_RECORD, recording_id); }
bool worry_seed_audio_play(uint16_t recording_id) { return recording_id && send(AUDIO_CMD_PLAY, recording_id); }
bool worry_seed_audio_stop(void) { return s_busy && send(AUDIO_CMD_STOP, 0); }
bool worry_seed_audio_busy(void) { return s_busy; }
