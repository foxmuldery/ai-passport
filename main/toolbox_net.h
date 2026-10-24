#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#include "toolbox_model.h"

typedef enum {
    TB_NET_BOOT = 0,
    TB_NET_CONNECTING,
    TB_NET_ONLINE,
    TB_NET_OFFLINE,
    TB_NET_WIFI_FAILED,
    TB_NET_PROVISION
} tb_net_state_t;

esp_err_t tb_net_start(void);

/** Ask the worker to fetch immediately (safe from a button callback). */
void tb_net_request_refresh(void);

/** Screen-off mode: poll less often and let Wi-Fi stay in max modem sleep. */
void tb_net_set_low_power(bool low);

/** Consume the "new data" flag. */
bool tb_net_take_dirty(void);

tb_net_state_t tb_net_state(void);

const char *tb_net_state_text(void);

void tb_net_last_error(char *buf, size_t len);

bool tb_net_has_data(void);

bool tb_net_lock(void);

void tb_net_unlock(void);

const tb_summary_t *tb_net_summary(void);

/** Erase the stored Wi-Fi/server config and reboot into the portal. */
void tb_net_reset_provisioning(void);
