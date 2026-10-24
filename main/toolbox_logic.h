#pragma once

#include <stdbool.h>
#include <stddef.h>

/** Format a leaderboard score for the 240 px display. */
void tb_format_score(float score, char *buf, size_t buf_len);

/** Scale a value into a bar height, with a visible minimum for non-zero data. */
int tb_bar_height(int value, int max_value, int max_height, int min_height);

/** Find the maximum value in an array (0 when empty). */
int tb_max_int(const int *values, int count);

/** Copy "2026-10-23" (or an ISO timestamp) into "10-23". */
void tb_short_date(const char *iso_date, char *buf, size_t buf_len);

/** Clamp an integer. */
int tb_clamp_int(int value, int min_value, int max_value);
