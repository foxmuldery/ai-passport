#include "toolbox_portal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "toolbox_config.h"

static const char *TAG = "tb_portal";
static const char *AP_PASSWORD = "toolbox123";

static httpd_handle_t s_server;
static char s_ssid[32];
static bool s_active;

static const char *PAGE_HEAD =
    "<!doctype html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>AI Toolbox 配网</title><style>"
    "body{margin:0;background:#070b14;color:#e6edf7;font-family:-apple-system,'PingFang SC',sans-serif;"
    "display:flex;justify-content:center;padding:24px}"
    ".card{width:100%;max-width:420px;background:#0f1727;border:1px solid #22304a;border-radius:16px;padding:22px}"
    "h1{font-size:20px;margin:0 0 4px}p{color:#8fa3bf;font-size:13px;line-height:1.6;margin:0 0 18px}"
    "label{display:block;font-size:12px;color:#8fa3bf;margin:12px 0 5px}"
    "input{width:100%;box-sizing:border-box;background:#0a1321;border:1px solid #22304a;border-radius:9px;"
    "padding:10px;color:#e6edf7;font-size:14px}"
    "button{width:100%;margin-top:20px;padding:11px;border:0;border-radius:10px;font-size:14px;"
    "font-weight:650;background:#22d3ee;color:#04121a}"
    ".hint{font-size:11px;color:#61748f;margin-top:12px}"
    "</style></head><body><div class=\"card\"><h1>AI Toolbox</h1>"
    "<p>把设备连接到你的 Wi-Fi，并填写 VPS 上的工具箱地址。</p><form method=\"post\" action=\"/save\">"
    "<label>Wi-Fi 名称（2.4GHz）</label><input name=\"ssid\" maxlength=\"32\" required>"
    "<label>Wi-Fi 密码</label><input name=\"pass\" type=\"password\" maxlength=\"64\">"
    "<label>工具箱地址</label><input name=\"base\" placeholder=\"https://toolbox.example.com\" required>"
    "<label>设备 Token</label><input name=\"token\" maxlength=\"128\" required>"
    "<button type=\"submit\">保存并重启</button></form>"
    "<div class=\"hint\">地址填 VPS 上运行的服务，不要带结尾斜杠。Token 在服务器的 .env 里（API_TOKEN）。</div>"
    "</div></body></html>";

static void url_decode(const char *src, char *dst, size_t dst_len)
{
    size_t out = 0;
    while (*src != '\0' && out + 1 < dst_len) {
        if (*src == '%' && src[1] != '\0' && src[2] != '\0') {
            char hex[3] = {src[1], src[2], '\0'};
            char *end = NULL;
            long value = strtol(hex, &end, 16);
            if (end == hex + 2) {
                dst[out++] = (char)value;
                src += 3;
                continue;
            }
        }
        if (*src == '+') {
            dst[out++] = ' ';
            src++;
            continue;
        }
        dst[out++] = *src++;
    }
    dst[out] = '\0';
}

static bool form_get(const char *body, const char *key, char *out, size_t out_len)
{
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "%s=", key);
    const char *p = body;
    const size_t pattern_len = strlen(pattern);
    while (p != NULL && *p != '\0') {
        const char *hit = strstr(p, pattern);
        if (hit == NULL) {
            break;
        }
        if (hit == body || hit[-1] == '&') {
            const char *value = hit + pattern_len;
            const char *end = strchr(value, '&');
            size_t value_len = end == NULL ? strlen(value) : (size_t)(end - value);
            if (value_len >= out_len) {
                value_len = out_len - 1;
            }
            char encoded[256];
            if (value_len >= sizeof(encoded)) {
                value_len = sizeof(encoded) - 1;
            }
            memcpy(encoded, value, value_len);
            encoded[value_len] = '\0';
            url_decode(encoded, out, out_len);
            return out[0] != '\0';
        }
        p = hit + 1;
    }
    return false;
}

static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, PAGE_HEAD, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t save_post(httpd_req_t *req)
{
    char body[1024];
    int received = httpd_req_recv(req, body, sizeof(body) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }
    body[received] = '\0';

    tb_config_t config;
    memset(&config, 0, sizeof(config));
    bool ok = true;
    ok = ok && form_get(body, "ssid", config.ssid, sizeof(config.ssid));
    (void)form_get(body, "pass", config.password, sizeof(config.password));
    ok = ok && form_get(body, "base", config.base_url, sizeof(config.base_url));
    ok = ok && form_get(body, "token", config.token, sizeof(config.token));

    if (!ok || !tb_config_valid(&config)) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        httpd_resp_sendstr(req,
            "<!doctype html><meta charset=\"utf-8\"><body style=\"background:#070b14;color:#ffc5cd;"
            "font-family:sans-serif;padding:24px\"><h2>配置不完整</h2>"
            "<p>请返回上一页，填写 Wi-Fi 名称、服务器地址和 Token。</p></body>");
        return ESP_FAIL;
    }

    if (!tb_config_save(&config)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NVS save failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_sendstr(req,
        "<!doctype html><meta charset=\"utf-8\"><body style=\"background:#070b14;color:#e6edf7;"
        "font-family:sans-serif;padding:24px\"><h2>已保存</h2>"
        "<p>设备马上重启并连接 Wi-Fi。</p></body>");
    ESP_LOGI(TAG, "configuration saved, restarting");
    vTaskDelay(pdMS_TO_TICKS(1200));
    esp_restart();
    return ESP_OK;
}

esp_err_t tb_portal_start(void)
{
    if (s_active) {
        return ESP_OK;
    }

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(s_ssid, sizeof(s_ssid), "AI-Toolbox-%02X%02X", mac[4], mac[5]);

    if (esp_netif_create_default_wifi_ap() == NULL) {
        ESP_LOGE(TAG, "failed to create AP netif");
        return ESP_FAIL;
    }

    wifi_config_t ap = {0};
    strncpy((char *)ap.ap.ssid, s_ssid, sizeof(ap.ap.ssid) - 1);
    ap.ap.ssid_len = (uint8_t)strlen(s_ssid);
    strncpy((char *)ap.ap.password, AP_PASSWORD, sizeof(ap.ap.password) - 1);
    ap.ap.channel = 6;
    ap.ap.max_connection = 2;
    ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap.ap.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());

    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    http_config.max_uri_handlers = 4;
    http_config.stack_size = 6144;
    http_config.lru_purge_enable = true;
    esp_err_t err = httpd_start(&s_server, &http_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
    httpd_uri_t save = {.uri = "/save", .method = HTTP_POST, .handler = save_post};
    httpd_register_uri_handler(s_server, &root);
    httpd_register_uri_handler(s_server, &save);

    s_active = true;
    ESP_LOGI(TAG, "portal ready: SSID=%s password=%s http://192.168.4.1/", s_ssid, AP_PASSWORD);
    return ESP_OK;
}

bool tb_portal_active(void)
{
    return s_active;
}

const char *tb_portal_ssid(void)
{
    return s_ssid;
}
