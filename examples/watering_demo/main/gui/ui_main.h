/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_style_t style;
    lv_style_t style_focus_no_outline;
    lv_style_t style_focus;
    lv_style_t style_pr;
} button_style_t;

void ui_main(void);
lv_group_t *ui_get_btn_op_group(void);
button_style_t *ui_button_styles(void);
lv_obj_t *ui_main_get_status_bar(void);
void ui_main_status_bar_set_wifi(bool is_connected);
void ui_watering_update_device_name(const char *name);

#ifdef __cplusplus
}
#endif
