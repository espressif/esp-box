/**
 * @file ui_dev_ctrl.c
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
#include <stdint.h>
#include "app_led.h"
#include "lvgl.h"
#include "ui_main.h"

LV_IMG_DECLARE(fan_on)
LV_IMG_DECLARE(fan_off)
LV_IMG_DECLARE(light_on)
LV_IMG_DECLARE(light_off)
LV_IMG_DECLARE(media_on)
LV_IMG_DECLARE(media_off)
LV_IMG_DECLARE(security_on)
LV_IMG_DECLARE(security_off)

LV_FONT_DECLARE(font_en_16)

typedef struct {
    const char *name;
    lv_img_dsc_t const* img_on;
    lv_img_dsc_t const* img_off;
} btn_img_src_t;

static lv_obj_t *obj_dev[4] = { [0 ... 3] = NULL };

static void btn_dev_cb(lv_event_t *event)
{
    if (LV_EVENT_VALUE_CHANGED != event->code && LV_EVENT_REFRESH != event->code) {
        return;
    }

    led_state_t led_state;
    lv_obj_t *btn = (lv_obj_t *) event->target;
    lv_obj_t *img = (lv_obj_t *) event->user_data;
    lv_obj_t *lab = (lv_obj_t *) btn->user_data;
    btn_img_src_t *p_img_src = (btn_img_src_t *) img->user_data;

    /* This is because event was triggered by another LED change API */
    if (LV_EVENT_REFRESH == event->code) {
        app_led_get_state(&led_state);
        if (led_state.on) {
            btn->state |= LV_STATE_CHECKED;
        } else {
            btn->state &= ~LV_STATE_CHECKED;
        }
    }

    if (btn->state & LV_STATE_CHECKED) {
        lv_img_set_src(img, p_img_src->img_on);
        lv_obj_set_style_text_color(lab, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(btn, 20, LV_STATE_CHECKED);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_20, LV_STATE_CHECKED);
    } else {
        lv_img_set_src(img, p_img_src->img_off);
        lv_obj_set_style_text_color(lab, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(btn, 0, LV_STATE_DEFAULT);
    }

    if (LV_EVENT_VALUE_CHANGED == event->code) {
        if (btn == obj_dev[0]) {
            if (btn->state & LV_STATE_CHECKED) {
                app_pwm_led_set_all_hsv(led_state.h, led_state.s, led_state.v);
            } else {
                app_pwm_led_set_all_hsv(0, 0, 0);
            }
        }
    }
}

static void btn_dev_detail_cb(lv_event_t *event)
{
    int32_t dev_id = (int32_t) event->user_data;

    if (dev_id == 0) {
        ui_dev_ctrl(false);
        ui_led(true);
    }
}

static void btn_back_dev_ctrl_cb(lv_event_t *event)
{
    ui_dev_ctrl(false);

    ui_clock(true);
}

void ui_dev_ctrl(bool show)
{
    static lv_obj_t *btn_back = NULL;
    static const btn_img_src_t img_src_list[] = {
        { .name = "Light", .img_on = &light_on, .img_off = &light_off },
        { .name = "Media", .img_on = &media_on, .img_off = &media_off },
        { .name = "Fan", .img_on = &fan_on, .img_off = &fan_off },
        { .name = "Security", .img_on = &security_on, .img_off = &security_off },
    };

    for (size_t i = 0; i < 4; i++) {
        if (NULL == obj_dev[i]) {
            obj_dev[i] = lv_btn_create(lv_scr_act());
            lv_obj_add_flag(obj_dev[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(obj_dev[i], 85, 85);
            lv_obj_set_style_bg_color(obj_dev[i], lv_color_white(), LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj_dev[i], lv_color_white(), LV_STATE_CHECKED);
            
            lv_obj_set_style_radius(obj_dev[i], 10, LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj_dev[i], 0, LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj_dev[i], 0, LV_STATE_DEFAULT);
            lv_obj_align(obj_dev[i], LV_ALIGN_CENTER, i % 2 ? 48 : -48, i < 2 ? -48 + 5 : 48 + 5);

            lv_obj_t *img = lv_img_create(obj_dev[i]);
            lv_img_set_src(img, img_src_list[i].img_off);
            lv_obj_align(img, LV_ALIGN_CENTER, 0, -10);
            lv_obj_set_user_data(img, (void *) &img_src_list[i]);

            lv_obj_t *label = lv_label_create(obj_dev[i]);
            lv_label_set_text_static(label, img_src_list[i].name);
            lv_obj_set_style_text_color(label, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(label, &font_en_16, LV_STATE_DEFAULT);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);

            lv_obj_set_user_data(obj_dev[i], (void *) label);
            lv_obj_add_event_cb(obj_dev[i], btn_dev_cb, LV_EVENT_ALL, (void *) img);
            lv_obj_add_event_cb(obj_dev[i], btn_dev_detail_cb, LV_EVENT_LONG_PRESSED, (void *) i);
        }
    }

    if (NULL == btn_back) {
        btn_back = lv_label_create(lv_scr_act());
        lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(btn_back, 20);
        lv_label_set_text_static(btn_back, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_font(btn_back, &lv_font_montserrat_16, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_back, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_back, lv_color_white(), LV_STATE_PRESSED);
        lv_obj_align(btn_back, LV_ALIGN_CENTER, -120, -48 - 20);
        lv_obj_add_event_cb(btn_back, btn_back_dev_ctrl_cb, LV_EVENT_CLICKED, NULL);
    }

    if (show) {
        // lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(237, 238, 239), LV_STATE_DEFAULT);
        lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(btn_back);
        for (size_t i = 0; i < 4; i++) {
            lv_obj_clear_flag(obj_dev[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_background(obj_dev[i]);
        }

        /* The LED status might changed by voice command */
        ui_dev_ctrl_update_state();
    } else {
        lv_obj_add_flag(btn_back, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < 4; i++) {
            lv_obj_add_flag(obj_dev[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_dev_ctrl_update_state(void)
{
    if (NULL != obj_dev[0]) {
        lv_event_send(obj_dev[0], LV_EVENT_REFRESH, NULL);
    }
}
