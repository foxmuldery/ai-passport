#pragma once

#include <stdbool.h>
#include <stddef.h>

#define TB_MAX_EXPIRY 6
#define TB_MAX_BOARDS 6
#define TB_MAX_TOP 10
#define TB_MAX_SITES 3
#define TB_NAME_MAX 40
#define TB_MODEL_MAX 56
#define TB_ORG_MAX 32
#define TB_DATE_MAX 12
#define TB_BADGE_MAX 24
#define TB_TIER_MAX 10
#define TB_KIND_MAX 10
#define TB_ERROR_MAX 96

typedef struct {
    char name[TB_NAME_MAX];
    char date[TB_DATE_MAX];
    int days;
    char badge[TB_BADGE_MAX];
    char tier[TB_TIER_MAX];
    char kind[TB_KIND_MAX];
    int amount;
} tb_expiry_t;

typedef struct {
    int rank;
    char model[TB_MODEL_MAX];
    char org[TB_ORG_MAX];
    float score;
} tb_entry_t;

typedef struct {
    char id[24];
    char name[TB_NAME_MAX];
    char updated[8];
    bool error;
    int top_count;
    tb_entry_t top[TB_MAX_TOP];
} tb_board_t;

typedef struct {
    char name[TB_NAME_MAX];
    int pv;
    int visits;
    int week;
    int month;
    int trend[7];
    int trend_count;
    int error;
} tb_site_t;

typedef struct {
    char clock[8];
    char date[TB_DATE_MAX];
    char site_name[TB_NAME_MAX];
    int pv;
    int visits;
    int week;
    int month;
    int trend[7];
    int trend_count;
    int site_count;
    tb_site_t sites[TB_MAX_SITES];
    int site_errors;
    int expiry_count;
    tb_expiry_t expiry[TB_MAX_EXPIRY];
    int board_count;
    tb_board_t boards[TB_MAX_BOARDS];
    int alerts_overdue;
    int alerts_due3;
    int board_errors;
} tb_summary_t;

/**
 * Parse the server's compact /api/device/summary payload into `out`.
 * Returns false and fills `err` when the document does not match the contract.
 */
bool tb_parse_summary(const char *json, size_t len, tb_summary_t *out, char *err, size_t err_len);
