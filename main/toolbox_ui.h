#pragma once

#include "esp_err.h"

#include "bsp_button.h"

/** Build the application UI and start its timers. */
esp_err_t tb_ui_start(void);

/** Thread-safe: enqueue a key event from the BSP callback. */
void tb_ui_key_event(bsp_btn_t btn, bsp_btn_ev_t ev);

/** Debug/capture helper: switch to page 0..3, waking the screen first. */
void tb_ui_show_page(int page);
