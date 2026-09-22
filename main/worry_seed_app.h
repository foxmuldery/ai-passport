#pragma once

#include "bsp_button.h"
#include "esp_err.h"

esp_err_t worry_seed_app_start(void);

// Enqueues one semantic USB debug action. The app task expands it into the
// same PRESS + CLICK/LONG sequence used by the physical buttons.
esp_err_t worry_seed_app_inject_button(bsp_btn_t button, bsp_btn_ev_t action);
