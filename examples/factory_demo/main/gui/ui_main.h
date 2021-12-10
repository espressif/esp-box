/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "main.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_style_t style;
    lv_style_t style_focus_no_outline;
    lv_style_t style_focus;
    lv_style_t style_pr;
} button_style_t;

esp_err_t ui_main_start(void);
void ui_acquire(void);
void ui_release(void);
lv_group_t *ui_get_btn_op_group(void);
lv_indev_t *ui_get_button_indev(void);
button_style_t *ui_button_styles(void);
lv_obj_t *ui_main_get_status_bar(void);
void ui_main_status_bar_set_wifi(bool is_connected);
void ui_main_status_bar_set_cloud(bool is_connected);
void ui_btn_rm_all_cb(void);

#ifdef __cplusplus
}
#endif
