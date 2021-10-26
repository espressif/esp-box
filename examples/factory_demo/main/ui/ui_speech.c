/**
 * @file ui_speech.c
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
#include "lvgl.h"
#include "ui_main.h"

LV_FONT_DECLARE(font_cmd_36)

static lv_timer_t *sr_timer = NULL;
static lv_obj_t *sr_mask = NULL;
static lv_obj_t *sr_bar[16] = { [0 ... 15] = NULL };

static int int16_sin(int32_t deg)
{
    static const int16_t sin_0_90_table[] = {
        0,     572,   1144,  1715,  2286,  2856,  3425,  3993,  4560,  5126,  5690,  6252,  6813,  7371,  7927,  8481,
        9032,  9580,  10126, 10668, 11207, 11743, 12275, 12803, 13328, 13848, 14364, 14876, 15383, 15886, 16383, 16876,
        17364, 17846, 18323, 18794, 19260, 19720, 20173, 20621, 21062, 21497, 21925, 22347, 22762, 23170, 23571, 23964,
        24351, 24730, 25101, 25465, 25821, 26169, 26509, 26841, 27165, 27481, 27788, 28087, 28377, 28659, 28932, 29196,
        29451, 29697, 29934, 30162, 30381, 30591, 30791, 30982, 31163, 31335, 31498, 31650, 31794, 31927, 32051, 32165,
        32269, 32364, 32448, 32523, 32587, 32642, 32687, 32722, 32747, 32762, 32767
    };

    if (deg < 0) {
        deg = -deg;
    }

    deg = deg % 360;

    if (deg <= 90) {
        return sin_0_90_table[deg];
    } else if (deg <= 180) {
        return sin_0_90_table[180 - deg];
    } else if (deg <= 270) {
        return -sin_0_90_table[deg - 270];
    } else {
        return -sin_0_90_table[360 - deg];
    }
}

static void sr_anim_cb(lv_timer_t *timer)
{
    lv_obj_t **bar = (lv_obj_t **) timer->user_data;
    static int32_t count = 0;

    lv_obj_move_foreground(sr_mask);

    for (size_t i = 0; i < 16; i++) {
        lv_bar_set_value(bar[i], 80 * abs(int16_sin(20 * i + count * 10)) / 36767 + 10, LV_ANIM_ON);
        lv_bar_set_start_value(bar[i], -80 * abs(int16_sin(20 * i + count * 10)) / 36767 - 10, LV_ANIM_ON);
    }

    count++;
}

static void sr_anim_hide_cb(lv_timer_t *timer)
{
    lv_obj_add_flag(sr_mask, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(timer);
}

void sr_anim_start(void)
{
    if (NULL == sr_mask) {
        sr_mask = lv_obj_create(lv_scr_act());
        lv_obj_set_size(sr_mask, 320, 210);
        lv_obj_clear_flag(sr_mask, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(sr_mask, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(sr_mask, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(sr_mask, lv_color_make(237, 238, 239), LV_STATE_DEFAULT);
        lv_obj_align(sr_mask, LV_ALIGN_CENTER, 0, 15);
    }
    
    for (size_t i = 0; i < sizeof(sr_bar) / sizeof(sr_bar[0]); i++) {
        if (NULL == sr_bar[i]) {
            sr_bar[i] = lv_bar_create(sr_mask);
            lv_obj_set_size(sr_bar[i], 6, 160);
            lv_obj_set_style_anim_time(sr_bar[i], 500, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(sr_bar[i], lv_color_make(237, 238 , 239), LV_STATE_DEFAULT);
            lv_bar_set_range(sr_bar[i], -100, 100);
            lv_bar_set_value(sr_bar[i], 0, LV_ANIM_OFF);
            lv_bar_set_start_value(sr_bar[i], -0, LV_ANIM_OFF);
            lv_bar_set_value(sr_bar[i], 20, LV_ANIM_ON);
            lv_bar_set_start_value(sr_bar[i], -20, LV_ANIM_ON);
            lv_obj_align(sr_bar[i], LV_ALIGN_CENTER, (sizeof(sr_bar) / sizeof(sr_bar[0]) / 2 - i) * -15 , -10);
        } else {
            lv_bar_set_value(sr_bar[i], 20, LV_ANIM_ON);
            lv_bar_set_start_value(sr_bar[i], -20, LV_ANIM_ON);
        }
    }

    lv_obj_move_foreground(sr_mask);
    lv_obj_clear_flag(sr_mask, LV_OBJ_FLAG_HIDDEN);

    if (NULL != sr_timer) {
        sr_anim_stop();
    }

    sr_timer = lv_timer_create(sr_anim_cb, 500, &sr_bar);
}

void sr_anim_stop(void)
{
    if (NULL == sr_timer) {
        return;
    }

    /* Delete sr animate timer */
    lv_timer_del(sr_timer);
    sr_timer = NULL;

    for (size_t i = 0; i < 16; i++) {
        lv_bar_set_start_value(sr_bar[i], -10, LV_ANIM_ON);
        lv_bar_set_value(sr_bar[i], 10, LV_ANIM_ON);
    }

    lv_timer_create(sr_anim_hide_cb, 500, NULL);
}

void sr_anim_set_text(char *text)
{
    static lv_obj_t *label = NULL;

    if (NULL == label) {
        label = lv_label_create(sr_mask);
        lv_obj_set_style_text_font(label, &font_cmd_36, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 40);
    }

    lv_label_set_text_static(label, text);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 80);
}
