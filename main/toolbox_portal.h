#pragma once

#include <stdbool.h>

#include "esp_err.h"

/** Start the SoftAP + HTTP configuration portal. */
esp_err_t tb_portal_start(void);

/** True while the portal is running. */
bool tb_portal_active(void);

/** The generated SoftAP SSID (valid after tb_portal_start succeeds). */
const char *tb_portal_ssid(void);
