/**
 * @file ui_led_ctrl.c
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

#include <stdbool.h>
#include "app_led.h"
#include "lvgl.h"
#include "ui_main.h"

static void btn_color_cb(lv_event_t *event)
{
    lv_obj_t *color_sel = (lv_obj_t *) event->user_data;
    lv_obj_t *color_btn = (lv_obj_t *) event->target;
    lv_color_t color = lv_obj_get_style_bg_color(color_btn, LV_PART_MAIN);

    lv_colorwheel_set_rgb(color_sel, color);
    lv_event_send(color_sel, LV_EVENT_VALUE_CHANGED, NULL);
}

static void btn_back_led_ctrl_cb(lv_event_t *event)
{
    (void) event;

    ui_led(false);
    ui_dev_ctrl(true);
}

static void color_sel_cb(lv_event_t *event)
{
    lv_obj_t *color_sel = (lv_obj_t *) event->target;

    lv_color_t color = lv_colorwheel_get_rgb(color_sel);

    app_pwm_led_set_all(color.ch.red << 3, (color.ch.green_h << 5) + (color.ch.green_l << 2), color.ch.blue << 3);
}

void ui_led(bool show)
{
    static lv_obj_t *color_sel = NULL;
    if (NULL == color_sel) {
        color_sel = lv_colorwheel_create(lv_scr_act(), true);
        lv_obj_set_size(color_sel, 128, 128);
        lv_obj_set_style_arc_width(color_sel, 15, LV_STATE_DEFAULT);
        lv_obj_align(color_sel, LV_ALIGN_CENTER, 0, -20);
        lv_obj_add_event_cb(color_sel, color_sel_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }

    static lv_obj_t *btn[4] = { [0 ... 3] = NULL };
    static lv_color_t color_list[] = {
        LV_COLOR_MAKE(210, 50, 100),
        LV_COLOR_MAKE(255, 222, 129),
        LV_COLOR_MAKE(128, 197, 255),
        LV_COLOR_MAKE(255, 172, 52),
    };
    for (size_t i = 0; i < 4; i++) {
        if (NULL == btn[i]) {
            btn[i] = lv_btn_create(lv_scr_act());
            lv_obj_set_size(btn[i], 40, 40);
            lv_obj_set_style_radius(btn[i], 20, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn[i], color_list[i], LV_STATE_DEFAULT);
            lv_obj_align(btn[i], LV_ALIGN_CENTER, 60 * i - 90, 80);
            lv_obj_add_event_cb(btn[i], btn_color_cb, LV_EVENT_SHORT_CLICKED, (void *) color_sel);
        }
    }

    static lv_obj_t *btn_back = NULL;
    if (NULL == btn_back) {
        btn_back = lv_label_create(lv_scr_act());
        lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(btn_back, 15);
        lv_label_set_text_static(btn_back, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_font(btn_back, &lv_font_montserrat_16, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_back, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_back, lv_color_white(), LV_STATE_PRESSED);
        lv_obj_align(btn_back, LV_ALIGN_CENTER, -120, -48 - 20);
        lv_obj_add_event_cb(btn_back, btn_back_led_ctrl_cb, LV_EVENT_CLICKED, NULL);
    }

    if (show) {
        lv_obj_clear_flag(color_sel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < 4; i++) {
            lv_obj_clear_flag(btn[i], LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(color_sel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_back, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < 4; i++) {
            lv_obj_add_flag(btn[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}
