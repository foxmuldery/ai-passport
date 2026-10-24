#include "toolbox_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "toolbox_config.h"
#include "toolbox_portal.h"

static const char *TAG = "tb_net";

#define HTTP_BUF_SIZE (24 * 1024)
#define FETCH_INTERVAL_MS (4 * 60 * 60 * 1000)
#define IDLE_FETCH_INTERVAL_MS FETCH_INTERVAL_MS
#define WIFI_RETRY_MS 5000
#define WIFI_MAX_RETRIES 6

static tb_config_t s_config;
static tb_summary_t s_summary;
static SemaphoreHandle_t s_lock;
static TaskHandle_t s_task;
static esp_timer_handle_t s_retry_timer;
static volatile tb_net_state_t s_state = TB_NET_BOOT;
static volatile bool s_wifi_connected;
static volatile bool s_dirty;
static volatile bool s_low_power;
static volatile bool s_has_data;
static int s_retry_count;
static char s_last_error[TB_ERROR_MAX];
static time_t s_last_fetch;

static void set_error(const char *message)
{
    if (s_lock != NULL && xSemaphoreTake(s_lock, pdMS_TO_TICKS(200)) == pdTRUE) {
        snprintf(s_last_error, sizeof(s_last_error), "%s", message ? message : "");
        xSemaphoreGive(s_lock);
    } else {
        snprintf(s_last_error, sizeof(s_last_error), "%s", message ? message : "");
    }
}

static void retry_timer_cb(void *arg)
{
    (void)arg;
    if (s_wifi_connected || s_state == TB_NET_PROVISION) {
        return;
    }
    if (s_retry_count >= WIFI_MAX_RETRIES) {
        ESP_LOGE(TAG, "Wi-Fi failed after %d attempts", s_retry_count);
        s_state = TB_NET_WIFI_FAILED;
        set_error("WiFi 连接失败");
        return;
    }
    s_retry_count++;
    ESP_LOGW(TAG, "Wi-Fi retry %d/%d", s_retry_count, WIFI_MAX_RETRIES);
    esp_wifi_connect();
    esp_timer_start_once(s_retry_timer, WIFI_RETRY_MS * 1000ULL);
}

static void start_sntp(void)
{
    static bool started;
    if (started) {
        return;
    }
    started = true;
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "pool.ntp.org");
    esp_sntp_init();
    ESP_LOGI(TAG, "SNTP started");
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "station started, connecting");
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_connected = false;
        if (s_state == TB_NET_PROVISION) {
            return;
        }
        ESP_LOGW(TAG, "Wi-Fi disconnected");
        s_state = TB_NET_OFFLINE;
        set_error("WiFi 已断开");
        if (s_retry_timer != NULL) {
            esp_timer_stop(s_retry_timer);
            esp_timer_start_once(s_retry_timer, WIFI_RETRY_MS * 1000ULL);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_wifi_connected = true;
        s_retry_count = 0;
        s_state = TB_NET_CONNECTING;
        set_error("");
        if (s_retry_timer != NULL) {
            esp_timer_stop(s_retry_timer);
        }
        ESP_LOGI(TAG, "got IP, starting SNTP");
        start_sntp();
        if (s_task != NULL) {
            xTaskNotifyGive(s_task);
        }
    }
}

static esp_err_t wifi_common_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    return ESP_OK;
}

static esp_err_t wifi_start_sta(const tb_config_t *config)
{
    esp_netif_create_default_wifi_sta();

    wifi_config_t wifi = {0};
    strncpy((char *)wifi.sta.ssid, config->ssid, sizeof(wifi.sta.ssid) - 1);
    strncpy((char *)wifi.sta.password, config->password, sizeof(wifi.sta.password) - 1);
    wifi.sta.threshold.authmode = config->password[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi.sta.pmf_cfg.capable = true;
    wifi.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
    ESP_ERROR_CHECK(esp_wifi_start());
    // The device only needs one small JSON every 5-15 minutes. MAX_MODEM lets
    // the radio sleep between DTIM beacons instead of staying in MIN_MODEM.
    esp_err_t ps_err = esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    if (ps_err != ESP_OK) {
        ESP_LOGW(TAG, "wifi power save setup failed: %s", esp_err_to_name(ps_err));
    }
    return ESP_OK;
}

static bool wait_for_time(int seconds)
{
    for (int i = 0; i < seconds * 2; i++) {
        if (time(NULL) > 1700000000) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    return false;
}

static bool fetch_once(void)
{
    char url[TB_URL_MAX + 32];
    snprintf(url, sizeof(url), "%s/api/device/summary", s_config.base_url);

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
        .keep_alive_enable = false,
    };
    if (strncmp(url, "https://", 8) == 0) {
        config.crt_bundle_attach = esp_crt_bundle_attach;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        set_error("HTTP 初始化失败");
        return false;
    }

    char auth[TB_TOKEN_MAX + 16];
    snprintf(auth, sizeof(auth), "Bearer %s", s_config.token);
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Accept", "application/json");

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "open failed: %s", esp_err_to_name(err));
        set_error("连接服务器失败");
        esp_http_client_cleanup(client);
        return false;
    }

    esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        char message[64];
        if (status == 401) {
            snprintf(message, sizeof(message), "Token 无效 (401)");
        } else if (status == 404) {
            snprintf(message, sizeof(message), "接口不存在 (404)");
        } else {
            snprintf(message, sizeof(message), "服务器返回 %d", status);
        }
        set_error(message);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return false;
    }

    char *buffer = malloc(HTTP_BUF_SIZE);
    if (buffer == NULL) {
        set_error("内存不足");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return false;
    }

    int total = 0;
    while (total < HTTP_BUF_SIZE - 1) {
        int read = esp_http_client_read(client, buffer + total, HTTP_BUF_SIZE - 1 - total);
        if (read <= 0) {
            break;
        }
        total += read;
    }
    buffer[total] = '\0';
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (total == 0) {
        set_error("服务器返回空响应");
        free(buffer);
        return false;
    }
    if (total >= HTTP_BUF_SIZE - 1) {
        set_error("响应过大");
        free(buffer);
        return false;
    }

    tb_summary_t *parsed = calloc(1, sizeof(*parsed));
    if (parsed == NULL) {
        set_error("内存不足");
        free(buffer);
        return false;
    }
    char parse_error[TB_ERROR_MAX] = {0};
    if (!tb_parse_summary(buffer, (size_t)total, parsed, parse_error, sizeof(parse_error))) {
        ESP_LOGE(TAG, "parse failed: %s", parse_error);
        set_error(parse_error[0] != '\0' ? parse_error : "数据解析失败");
        free(parsed);
        free(buffer);
        return false;
    }
    free(buffer);

    if (s_lock != NULL && xSemaphoreTake(s_lock, pdMS_TO_TICKS(500)) == pdTRUE) {
        s_summary = *parsed;
        s_last_fetch = time(NULL);
        s_has_data = true;
        s_dirty = true;
        s_last_error[0] = '\0';
        xSemaphoreGive(s_lock);
    }
    ESP_LOGI(TAG, "summary updated: %d boards, %d expiry, fetched_at=%ld", parsed->board_count,
             parsed->expiry_count, (long)s_last_fetch);
    free(parsed);
    return true;
}

static void net_task(void *arg)
{
    (void)arg;
    bool first = true;
    for (;;) {
        if (!first) {
            const uint32_t interval = s_low_power ? IDLE_FETCH_INTERVAL_MS : FETCH_INTERVAL_MS;
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(interval));
        }
        first = false;
        if (s_state == TB_NET_PROVISION || !s_wifi_connected) {
            continue;
        }
        if (!wait_for_time(12)) {
            ESP_LOGW(TAG, "time not synced, skipping fetch");
            s_state = TB_NET_OFFLINE;
            set_error("等待网络时间");
            continue;
        }
        s_state = TB_NET_CONNECTING;
        if (fetch_once()) {
            s_state = TB_NET_ONLINE;
        } else {
            s_state = TB_NET_OFFLINE;
        }
    }
}

esp_err_t tb_net_start(void)
{
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
    }
    esp_err_t err = wifi_common_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi init failed: %s", esp_err_to_name(err));
        s_state = TB_NET_WIFI_FAILED;
        set_error("WiFi 初始化失败");
        return err;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = retry_timer_cb,
        .name = "tb_wifi_retry",
    };
    esp_timer_create(&timer_args, &s_retry_timer);

    if (!tb_config_load(&s_config)) {
        ESP_LOGW(TAG, "no configuration, starting portal");
        err = tb_portal_start();
        s_state = err == ESP_OK ? TB_NET_PROVISION : TB_NET_WIFI_FAILED;
        if (err != ESP_OK) {
            set_error("配网服务启动失败");
        }
        return err;
    }

    s_state = TB_NET_CONNECTING;
    err = wifi_start_sta(&s_config);
    if (err != ESP_OK) {
        s_state = TB_NET_WIFI_FAILED;
        set_error("WiFi 启动失败");
        return err;
    }

    if (xTaskCreate(net_task, "tb_net", 10240, NULL, 4, &s_task) != pdPASS) {
        s_state = TB_NET_WIFI_FAILED;
        set_error("网络任务创建失败");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void tb_net_request_refresh(void)
{
    if (s_task != NULL) {
        xTaskNotifyGive(s_task);
    }
}

void tb_net_set_low_power(bool low)
{
    if (s_low_power == low) {
        return;
    }
    s_low_power = low;
    // Waking the screen must not fetch: the UI keeps the last snapshot until
    // the user long-presses OK or the 4-hour timer fires.
    ESP_LOGI(TAG, "low power %s: auto fetch every %d min", low ? "on" : "off",
             (int)((low ? IDLE_FETCH_INTERVAL_MS : FETCH_INTERVAL_MS) / 60000));
}

bool tb_net_take_dirty(void)
{
    if (!s_dirty) {
        return false;
    }
    s_dirty = false;
    return true;
}

tb_net_state_t tb_net_state(void)
{
    return s_state;
}

const char *tb_net_state_text(void)
{
    switch (s_state) {
    case TB_NET_ONLINE:
        return "在线";
    case TB_NET_CONNECTING:
        return "连接中";
    case TB_NET_OFFLINE:
        return "离线";
    case TB_NET_WIFI_FAILED:
        return "WiFi";
    case TB_NET_PROVISION:
        return "配网";
    default:
        return "启动中";
    }
}

void tb_net_last_error(char *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return;
    }
    buf[0] = '\0';
    if (s_lock != NULL && xSemaphoreTake(s_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(buf, len, "%s", s_last_error);
        xSemaphoreGive(s_lock);
    }
}

bool tb_net_has_data(void)
{
    return s_has_data;
}

bool tb_net_lock(void)
{
    return s_lock != NULL && xSemaphoreTake(s_lock, pdMS_TO_TICKS(300)) == pdTRUE;
}

void tb_net_unlock(void)
{
    if (s_lock != NULL) {
        xSemaphoreGive(s_lock);
    }
}

const tb_summary_t *tb_net_summary(void)
{
    return &s_summary;
}

void tb_net_reset_provisioning(void)
{
    tb_config_clear();
    vTaskDelay(pdMS_TO_TICKS(300));
    esp_restart();
}
