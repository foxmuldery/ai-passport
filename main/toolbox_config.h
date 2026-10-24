#pragma once

#include <stdbool.h>
#include <stddef.h>

#define TB_SSID_MAX 33
#define TB_PASS_MAX 65
#define TB_URL_MAX 160
#define TB_TOKEN_MAX 160

typedef struct {
    char ssid[TB_SSID_MAX];
    char password[TB_PASS_MAX];
    char base_url[TB_URL_MAX];
    char token[TB_TOKEN_MAX];
} tb_config_t;

/** Load Wi-Fi + server configuration from NVS. Returns false when incomplete. */
bool tb_config_load(tb_config_t *out);

/** Persist the configuration to NVS. */
bool tb_config_save(const tb_config_t *cfg);

/** Remove the stored configuration (used by the reset gesture). */
void tb_config_clear(void);

/** True when all fields are present and the base URL looks usable. */
bool tb_config_valid(const tb_config_t *cfg);

/** Trim trailing slashes from a base URL in place. */
void tb_config_trim_base(char *base);
