/**
 * @file ui_clock.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-29
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <stdio.h>
#include "lvgl.h"
#include "ui_lang.h"
#include "ui_main.h"

LV_FONT_DECLARE(FONT_HINT)
LV_FONT_DECLARE(font_en_24)
LV_FONT_DECLARE(font_en_64)

static lv_obj_t *label_time = NULL;
static lv_obj_t *label_close = NULL;

static void btn_home_cb(lv_event_t *event)
{
    if (NULL != event->param) {
        ui_led(false);
        ui_network(false);
        ui_dev_ctrl(false);
        ui_clock(true);
    }
}

static void hint_close_cb(lv_event_t *event)
{
    ui_hint(false);
}

void ui_hint(bool show)
{
    static lv_obj_t *obj = NULL;
    if (NULL == obj) {
        obj = lv_obj_create(lv_scr_act());
        lv_obj_set_size(obj, 320, 160);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(obj, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_radius(obj, 15, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(obj, 100, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(obj, LV_OPA_30, LV_STATE_DEFAULT);
        lv_obj_center(obj);

        lv_obj_t *label_title = lv_label_create(obj);
        lv_label_set_text_static(label_title, STR_HINT_TITLE);
        lv_obj_set_style_text_color(label_title, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_title, &FONT_HINT, LV_STATE_DEFAULT);
        lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t *label_hint = lv_label_create(obj);
        lv_label_set_text_static(label_hint, STR_HINT_MSG);
        lv_obj_set_style_text_line_space(label_hint, 6, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_hint, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_hint, &FONT_HINT, LV_STATE_DEFAULT);
        lv_obj_align(label_hint, LV_ALIGN_CENTER, 0, 0);

        label_close = lv_label_create(obj);
        lv_label_set_text_static(label_close, STR_HINT_OK);
        lv_obj_set_ext_click_area(label_close, 20);
        lv_obj_add_flag(label_close, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(label_close, lv_color_make(251, 17, 26), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_close, lv_color_black(), LV_STATE_PRESSED);
        lv_obj_set_style_text_font(label_close, &FONT_HINT, LV_STATE_DEFAULT);
        lv_obj_align(label_close, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_event_cb(label_close, hint_close_cb, LV_EVENT_CLICKED, (void *) obj);
    }

    if (show) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_clock(bool show)
{
    static lv_obj_t *panel = NULL;
    if (NULL == panel) {
        panel = lv_obj_create(lv_scr_act());
        lv_obj_set_style_radius(panel, 20, 0);
        lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
        lv_obj_set_style_border_width(panel, 0, 0);
        lv_obj_set_style_shadow_width(panel, 20, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(panel, LV_OPA_10, LV_STATE_DEFAULT);
        lv_obj_set_size(panel, 290, 180);
        lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, -15);

        lv_obj_add_event_cb(lv_scr_act(), btn_home_cb, LV_EVENT_HIT_TEST, NULL);
    }

    if (NULL == label_time) {
        label_time = lv_label_create(panel);
        lv_label_set_text_static(label_time, "00:00");
        lv_obj_set_style_text_font(label_time, &font_en_64, 0);
        lv_obj_set_style_text_color(label_time, lv_color_make(40, 40, 40), 0);
        lv_obj_align(label_time, LV_ALIGN_CENTER, 0, -20);
    }

    static lv_obj_t *label_date = NULL;
    if (NULL == label_date) {
        label_date = lv_label_create(panel);
        lv_label_set_text_static(label_date, "Thursday, October 14");
        lv_obj_set_style_text_font(label_date, &font_en_24, 0);
        lv_obj_set_style_text_color(label_date, lv_color_make(80, 80, 80), 0);
        lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 30);
    }

    if (show) {
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_clock_set_time(char *text)
{
    if (NULL != label_time) {
        lv_label_set_text_static(label_time, text);
    }
}
