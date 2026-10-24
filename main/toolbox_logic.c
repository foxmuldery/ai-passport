#include "toolbox_logic.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void tb_format_score(float score, char *buf, size_t buf_len)
{
    if (buf == NULL || buf_len == 0) {
        return;
    }
    if (!isfinite(score)) {
        snprintf(buf, buf_len, "—");
        return;
    }
    const float rounded = roundf(score);
    if (fabsf(score - rounded) < 0.05f) {
        snprintf(buf, buf_len, "%.0f", score);
    } else if (score >= 10.0f) {
        snprintf(buf, buf_len, "%.1f", score);
    } else {
        snprintf(buf, buf_len, "%.2f", score);
    }
}

int tb_bar_height(int value, int max_value, int max_height, int min_height)
{
    if (value <= 0) {
        return 0;
    }
    if (max_value <= 0 || max_height <= 0) {
        return min_height;
    }
    int height = (int)lroundf(((float)value / (float)max_value) * (float)max_height);
    if (height < min_height) {
        height = min_height;
    }
    if (height > max_height) {
        height = max_height;
    }
    return height;
}

int tb_max_int(const int *values, int count)
{
    int max_value = 0;
    if (values == NULL) {
        return 0;
    }
    for (int i = 0; i < count; i++) {
        if (values[i] > max_value) {
            max_value = values[i];
        }
    }
    return max_value;
}

void tb_short_date(const char *iso_date, char *buf, size_t buf_len)
{
    if (buf == NULL || buf_len == 0) {
        return;
    }
    if (iso_date == NULL || strlen(iso_date) < 10) {
        snprintf(buf, buf_len, "—");
        return;
    }
    snprintf(buf, buf_len, "%.5s", iso_date + 5);
}

int tb_clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}
