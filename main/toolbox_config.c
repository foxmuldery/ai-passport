#include "toolbox_config.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "tb_config";
static const char *NVS_NAMESPACE = "tbcfg";

static void copy_field(char *dst, size_t dst_len, const char *src)
{
    if (dst_len == 0) {
        return;
    }
    snprintf(dst, dst_len, "%s", src ? src : "");
}

bool tb_config_valid(const tb_config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }
    if (cfg->ssid[0] == '\0' || cfg->token[0] == '\0' || cfg->base_url[0] == '\0') {
        return false;
    }
    if (strncmp(cfg->base_url, "http://", 7) != 0 && strncmp(cfg->base_url, "https://", 8) != 0) {
        return false;
    }
    return true;
}

bool tb_config_load(tb_config_t *out)
{
    if (out == NULL) {
        return false;
    }
    memset(out, 0, sizeof(*out));

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return false;
    }

    size_t len = sizeof(out->ssid);
    (void)nvs_get_str(handle, "ssid", out->ssid, &len);
    len = sizeof(out->password);
    (void)nvs_get_str(handle, "pass", out->password, &len);
    len = sizeof(out->base_url);
    (void)nvs_get_str(handle, "base", out->base_url, &len);
    len = sizeof(out->token);
    (void)nvs_get_str(handle, "token", out->token, &len);
    nvs_close(handle);

    tb_config_trim_base(out->base_url);
    if (!tb_config_valid(out)) {
        ESP_LOGW(TAG, "stored config is incomplete (ssid=%d base=%d token=%d)",
                 out->ssid[0] != '\0', out->base_url[0] != '\0', out->token[0] != '\0');
        return false;
    }
    ESP_LOGI(TAG, "config loaded: ssid=%s base=%s", out->ssid, out->base_url);
    return true;
}

bool tb_config_save(const tb_config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }
    tb_config_t normalized = *cfg;
    tb_config_trim_base(normalized.base_url);
    if (!tb_config_valid(&normalized)) {
        ESP_LOGE(TAG, "refusing to save incomplete config");
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_set_str(handle, "ssid", normalized.ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, "pass", normalized.password);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, "base", normalized.base_url);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, "token", normalized.token);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "saving config failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "config saved");
    return true;
}

void tb_config_clear(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return;
    }
    (void)nvs_erase_key(handle, "ssid");
    (void)nvs_erase_key(handle, "pass");
    (void)nvs_erase_key(handle, "base");
    (void)nvs_erase_key(handle, "token");
    (void)nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGW(TAG, "config cleared");
}

void tb_config_trim_base(char *base)
{
    if (base == NULL) {
        return;
    }
    size_t len = strlen(base);
    while (len > 0 && base[len - 1] == '/') {
        base[--len] = '\0';
    }
}
