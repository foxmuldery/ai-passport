#include "toolbox_model.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "tb_model";

static void set_error(char *err, size_t err_len, const char *message)
{
    if (err != NULL && err_len > 0) {
        snprintf(err, err_len, "%s", message);
    }
}

static void copy_string(char *dst, size_t dst_len, const cJSON *item)
{
    if (dst_len == 0) {
        return;
    }
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        snprintf(dst, dst_len, "%s", item->valuestring);
    } else {
        dst[0] = '\0';
    }
}

static int int_value(const cJSON *object, const char *key, int fallback)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsNumber(item)) {
        return (int)item->valuedouble;
    }
    return fallback;
}

static double double_value(const cJSON *object, const char *key, double fallback)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsNumber(item)) {
        return item->valuedouble;
    }
    return fallback;
}

static bool array_value(const cJSON *object, const char *key, const cJSON **out, int *count)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsArray(item)) {
        return false;
    }
    *out = item;
    *count = cJSON_GetArraySize(item);
    return true;
}

static void copy_name(char *dst, size_t dst_len, const cJSON *object, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    copy_string(dst, dst_len, item);
}

bool tb_parse_summary(const char *json, size_t len, tb_summary_t *out, char *err, size_t err_len)
{
    if (out == NULL) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    if (err != NULL && err_len > 0) {
        err[0] = '\0';
    }
    if (json == NULL || len == 0) {
        set_error(err, err_len, "empty response");
        return false;
    }

    cJSON *root = cJSON_ParseWithLength(json, len);
    if (root == NULL) {
        set_error(err, err_len, "invalid JSON");
        return false;
    }

    bool ok = false;
    const cJSON *site = cJSON_GetObjectItemCaseSensitive(root, "site");
    const cJSON *expiry = cJSON_GetObjectItemCaseSensitive(root, "exp");
    const cJSON *boards = cJSON_GetObjectItemCaseSensitive(root, "brd");
    if (!cJSON_IsObject(site) || !cJSON_IsArray(expiry) || !cJSON_IsArray(boards)) {
        set_error(err, err_len, "missing site/exp/brd");
        goto done;
    }

    copy_name(out->clock, sizeof(out->clock), root, "t");
    copy_name(out->date, sizeof(out->date), root, "d");
    copy_name(out->site_name, sizeof(out->site_name), site, "n");
    out->pv = int_value(site, "pv", 0);
    out->visits = int_value(site, "v", 0);
    out->week = int_value(site, "w", 0);
    out->month = int_value(site, "m", 0);
    out->site_errors = int_value(site, "e", 0);

    const cJSON *trend = cJSON_GetObjectItemCaseSensitive(site, "tr");
    if (cJSON_IsArray(trend)) {
        int count = cJSON_GetArraySize(trend);
        if (count > 7) {
            count = 7;
        }
        for (int i = 0; i < count; i++) {
            const cJSON *item = cJSON_GetArrayItem(trend, i);
            if (cJSON_IsNumber(item)) {
                out->trend[out->trend_count++] = (int)item->valuedouble;
            }
        }
    }

    const cJSON *site_list = cJSON_GetObjectItemCaseSensitive(site, "s");
    if (cJSON_IsArray(site_list)) {
        int count = cJSON_GetArraySize(site_list);
        if (count > TB_MAX_SITES) {
            count = TB_MAX_SITES;
        }
        for (int i = 0; i < count; i++) {
            const cJSON *row = cJSON_GetArrayItem(site_list, i);
            if (!cJSON_IsObject(row)) {
                continue;
            }
            tb_site_t *st = &out->sites[out->site_count];
            copy_name(st->name, sizeof(st->name), row, "n");
            st->pv = int_value(row, "pv", 0);
            st->visits = int_value(row, "v", 0);
            st->week = int_value(row, "w", 0);
            st->month = int_value(row, "m", 0);
            st->error = int_value(row, "e", 0) != 0;
            const cJSON *site_trend = cJSON_GetObjectItemCaseSensitive(row, "tr");
            if (cJSON_IsArray(site_trend)) {
                int n = cJSON_GetArraySize(site_trend);
                if (n > 7) {
                    n = 7;
                }
                for (int j = 0; j < n; j++) {
                    const cJSON *item = cJSON_GetArrayItem(site_trend, j);
                    if (cJSON_IsNumber(item)) {
                        st->trend[st->trend_count++] = (int)item->valuedouble;
                    }
                }
            }
            if (st->name[0] != '\0') {
                out->site_count++;
            }
        }
    }

    int exp_count = cJSON_GetArraySize(expiry);
    if (exp_count > TB_MAX_EXPIRY) {
        exp_count = TB_MAX_EXPIRY;
    }
    for (int i = 0; i < exp_count; i++) {
        const cJSON *row = cJSON_GetArrayItem(expiry, i);
        if (!cJSON_IsObject(row)) {
            continue;
        }
        tb_expiry_t *e = &out->expiry[out->expiry_count];
        copy_name(e->name, sizeof(e->name), row, "n");
        copy_name(e->date, sizeof(e->date), row, "dt");
        copy_name(e->badge, sizeof(e->badge), row, "b");
        copy_name(e->tier, sizeof(e->tier), row, "t");
        copy_name(e->kind, sizeof(e->kind), row, "k");
        e->days = int_value(row, "d", 0);
        e->amount = int_value(row, "a", 0);
        if (e->name[0] != '\0') {
            out->expiry_count++;
        }
    }

    int board_count = cJSON_GetArraySize(boards);
    if (board_count > TB_MAX_BOARDS) {
        board_count = TB_MAX_BOARDS;
    }
    for (int i = 0; i < board_count; i++) {
        const cJSON *row = cJSON_GetArrayItem(boards, i);
        if (!cJSON_IsObject(row)) {
            continue;
        }
        tb_board_t *b = &out->boards[out->board_count];
        copy_name(b->id, sizeof(b->id), row, "id");
        copy_name(b->name, sizeof(b->name), row, "n");
        copy_name(b->updated, sizeof(b->updated), row, "u");
        b->error = int_value(row, "e", 0) != 0;
        const cJSON *top = cJSON_GetObjectItemCaseSensitive(row, "top");
        if (cJSON_IsArray(top)) {
            int top_count = cJSON_GetArraySize(top);
            if (top_count > TB_MAX_TOP) {
                top_count = TB_MAX_TOP;
            }
            for (int j = 0; j < top_count; j++) {
                const cJSON *entry = cJSON_GetArrayItem(top, j);
                if (!cJSON_IsObject(entry)) {
                    continue;
                }
                tb_entry_t *t = &b->top[b->top_count];
                t->rank = int_value(entry, "r", j + 1);
                copy_name(t->model, sizeof(t->model), entry, "m");
                copy_name(t->org, sizeof(t->org), entry, "o");
                t->score = (float)double_value(entry, "s", 0.0);
                if (t->model[0] != '\0') {
                    b->top_count++;
                }
            }
        }
        if (b->name[0] != '\0' || b->id[0] != '\0') {
            out->board_count++;
        }
    }

    const cJSON *alerts = cJSON_GetObjectItemCaseSensitive(root, "alerts");
    if (cJSON_IsObject(alerts)) {
        out->alerts_overdue = int_value(alerts, "overdue", 0);
        out->alerts_due3 = int_value(alerts, "due3", 0);
        out->board_errors = int_value(alerts, "boardErrors", 0);
    }

    ok = true;
    ESP_LOGI(TAG, "parsed: exp=%d boards=%d pv=%d", out->expiry_count, out->board_count, out->pv);

done:
    cJSON_Delete(root);
    return ok;
}
