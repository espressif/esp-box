/**
 * @file ui_status_bar.c
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

#include "lvgl.h"
#include "ui_main.h"

LV_FONT_DECLARE(font_en_16)
LV_FONT_DECLARE(font_app)

static lv_obj_t *label_clock = NULL;
static lv_obj_t *label_wifi = NULL;

static void btn_app_cb(lv_event_t *event)
{
    (void) event;

    ui_clock(false);
    ui_network(false);
    ui_led(false);
    ui_hint(false);
    ui_dev_ctrl(true);
}

static void btn_network_cb(lv_event_t *event)
{
    ui_clock(false);
    ui_dev_ctrl(false);
    ui_led(false);
    ui_hint(false);
    ui_network(true);
}

static void btn_clock_cb(lv_event_t *event)
{
    ui_network(false);
    ui_dev_ctrl(false);
    ui_led(false);
    ui_hint(false);
    ui_clock(true);
}

void ui_status_bar_init(lv_color_t color)
{
    if (NULL == label_clock) {
        label_clock = lv_label_create(lv_scr_act());
        lv_obj_add_flag(label_clock, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(label_clock, 15);
        lv_label_set_text_static(label_clock, "08:00");
        lv_obj_set_style_text_font(label_clock, &font_en_16, 0);
        lv_obj_set_style_text_color(label_clock, color, 0);
        lv_obj_align(label_clock, LV_ALIGN_TOP_LEFT, 10, 5);
        lv_obj_add_event_cb(label_clock, btn_clock_cb, LV_EVENT_CLICKED, NULL);
    }

    if (NULL == label_wifi) {
        label_wifi = lv_label_create(lv_scr_act());
        lv_obj_add_flag(label_wifi, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text_static(label_wifi, LV_SYMBOL_WIFI);
        lv_obj_set_ext_click_area(label_wifi, 15);
        lv_obj_set_style_text_font(label_wifi, &lv_font_montserrat_16, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_wifi, color, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_wifi, lv_color_make(192, 192, 192), LV_STATE_PRESSED);
        lv_obj_align(label_wifi, LV_ALIGN_TOP_LEFT, 60, 5);
        lv_obj_add_event_cb(label_wifi, btn_network_cb, LV_EVENT_CLICKED, NULL);
    }

    static lv_obj_t *btn_app = NULL;
    if (NULL == btn_app) {
        btn_app = lv_label_create(lv_scr_act());
        lv_obj_add_flag(btn_app, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(btn_app, 20);
        lv_obj_set_style_text_font(btn_app, &font_app, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_app, color, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_app, lv_color_make(192, 192, 192), LV_STATE_PRESSED);
        lv_label_set_text_static(btn_app, "\xef\xa8\x80");
        lv_obj_align(btn_app, LV_ALIGN_TOP_RIGHT, -40, 7);
        lv_obj_add_event_cb(btn_app, btn_app_cb, LV_EVENT_CLICKED, NULL);
    }

    static lv_obj_t *btn_setting = NULL;
    if (NULL == btn_setting) {
        btn_setting = lv_label_create(lv_scr_act());
        lv_obj_add_flag(btn_setting, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_font(btn_setting, &lv_font_montserrat_16, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_setting, color, 0);
        lv_label_set_text_static(btn_setting, LV_SYMBOL_SETTINGS);
        lv_obj_align(btn_setting, LV_ALIGN_TOP_RIGHT, -10, 5);
    }

    lv_obj_set_style_text_color(label_clock, color, 0);
    lv_obj_set_style_text_color(label_wifi, color, 0);
    lv_obj_set_style_text_color(btn_app, color, 0);
    lv_obj_set_style_text_color(btn_setting, color, 0);
}

void ui_status_bar_set_time(char *text)
{
    if (NULL != label_clock) {
        lv_label_set_text_static(label_clock, text);
    }
}
