#include "worry_seed_store.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs.h"
#include "nvs_flash.h"

#define STORE_MAGIC 0x57534545U
#define STORE_VERSION 1U
#define STORE_NAMESPACE "worryseed"
#define STORE_KEY "garden"
#define WORRY_FS_BASE "/worry"

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    worry_seed_model_t model;
    uint32_t crc32;
} store_blob_t;

static const char *TAG = "worry_store";
static nvs_handle_t s_nvs;
static bool s_ready;

static uint32_t crc32_bytes(const void *data, size_t length) {
    const uint8_t *bytes = data;
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xedb88320U & (uint32_t)-(int32_t)(crc & 1U));
        }
    }
    return ~crc;
}

bool worry_seed_store_audio_path(uint16_t recording_id, bool temporary,
                                 char *path, size_t path_size) {
    if (!recording_id || !path || path_size < 24) return false;
    int written = snprintf(path, path_size, WORRY_FS_BASE "/r%05u.%s",
                           recording_id, temporary ? "tmp" : "wsad");
    return written > 0 && (size_t)written < path_size;
}

bool worry_seed_store_init(worry_seed_model_t *model) {
    if (!model) return false;
    worry_seed_model_defaults(model);
    if (s_ready) return true;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err == ESP_OK) err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_open(STORE_NAMESPACE, NVS_READWRITE, &s_nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return false;
    }

    const esp_vfs_spiffs_conf_t fs = {
        .base_path = WORRY_FS_BASE,
        .partition_label = "worryfs",
        .max_files = 4,
        .format_if_mount_failed = true,
    };
    err = esp_vfs_spiffs_register(&fs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
        nvs_close(s_nvs);
        s_nvs = 0;
        return false;
    }

    store_blob_t blob;
    size_t size = sizeof(blob);
    err = nvs_get_blob(s_nvs, STORE_KEY, &blob, &size);
    if (err == ESP_OK && size == sizeof(blob) && blob.magic == STORE_MAGIC &&
        blob.version == STORE_VERSION && blob.size == sizeof(blob) &&
        blob.crc32 == crc32_bytes(&blob, offsetof(store_blob_t, crc32)) &&
        worry_seed_model_validate(&blob.model)) {
        *model = blob.model;
        ESP_LOGI(TAG, "restored %u seeds", model->count);
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "stored garden invalid; using empty garden");
    }
    s_ready = true;
    return true;
}

bool worry_seed_store_save(const worry_seed_model_t *model) {
    if (!s_ready || !model) return false;
    store_blob_t blob = {
        .magic = STORE_MAGIC,
        .version = STORE_VERSION,
        .size = sizeof(store_blob_t),
        .model = *model,
    };
    blob.crc32 = crc32_bytes(&blob, offsetof(store_blob_t, crc32));
    esp_err_t err = nvs_set_blob(s_nvs, STORE_KEY, &blob, sizeof(blob));
    if (err == ESP_OK) err = nvs_commit(s_nvs);
    if (err != ESP_OK) ESP_LOGE(TAG, "save failed: %s", esp_err_to_name(err));
    return err == ESP_OK;
}

FILE *worry_seed_store_audio_open(uint16_t recording_id, const char *mode) {
    char path[32];
    bool temporary = mode && mode[0] == 'w';
    if (!worry_seed_store_audio_path(recording_id, temporary, path, sizeof(path))) return NULL;
    return fopen(path, mode);
}

bool worry_seed_store_audio_commit(uint16_t recording_id) {
    char temp[32], final[32];
    if (!worry_seed_store_audio_path(recording_id, true, temp, sizeof(temp)) ||
        !worry_seed_store_audio_path(recording_id, false, final, sizeof(final))) return false;
    (void)remove(final);
    return rename(temp, final) == 0;
}

bool worry_seed_store_audio_remove(uint16_t recording_id) {
    char temp[32], final[32];
    bool ok = worry_seed_store_audio_path(recording_id, true, temp, sizeof(temp)) &&
              worry_seed_store_audio_path(recording_id, false, final, sizeof(final));
    if (!ok) return false;
    int a = remove(temp);
    int b = remove(final);
    return a == 0 || b == 0;
}

size_t worry_seed_store_free_bytes(void) {
    size_t total = 0, used = 0;
    if (esp_spiffs_info("worryfs", &total, &used) != ESP_OK || used > total) return 0;
    return total - used;
}
