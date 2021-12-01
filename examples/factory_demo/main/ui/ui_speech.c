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
#include "ui_lang.h"
#include "ui_main.h"

LV_FONT_DECLARE(FONT_CMD)

static bool sr_anim_active = false;
static int32_t sr_anim_count = 0;
static lv_obj_t *sr_label = NULL;
static lv_obj_t *sr_mask = NULL;
static lv_obj_t *sr_bar[8] = { [0 ... 7] = NULL };

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

static void sr_label_event_handler(lv_event_t *event)
{
    char *text = (char *) event-> param;
    if (NULL != text) {
        lv_label_set_text_static(sr_label, text);
    }
}

static void sr_mask_event_handler(lv_event_t *event)
{
    bool active = (bool) event->param;

    if (active) {
        sr_anim_count = 0;
        sr_anim_active = true;
        lv_obj_clear_flag(sr_mask, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(sr_mask);
    } else {
        sr_anim_active = false;
    }
}

static void ui_speech_anim_cb(lv_timer_t *timer)
{
    const int32_t step = 40;

    if (sr_anim_active) {
        /* Will hide hint message after wakeup */
        static bool hint_hide = false;
        if (hint_hide != true) {
            ui_hint(false);
            hint_hide = true;
        }

        /* Show sr animate mask and move to foreground */
        if (lv_obj_has_flag(sr_mask, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(sr_mask, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(sr_mask);
        }

        /* Set bar value */
        int32_t sr_val[4] = {
            abs(int16_sin(sr_anim_count * step + step * 0)) / (32768 >> 7),
            abs(int16_sin(sr_anim_count * step + step * 1)) / (32768 >> 7),
            abs(int16_sin(sr_anim_count * step + step * 2)) / (32768 >> 7),
            abs(int16_sin(sr_anim_count * step + step * 3)) / (32768 >> 7),
        };

        for (size_t i = 0; i < 4; i++) {
            lv_bar_set_value(sr_bar[i], sr_val[i] > 20 ? sr_val[i] : 20, LV_ANIM_ON);
            lv_bar_set_value(sr_bar[7 - i], sr_val[i] > 20 ? sr_val[i] : 20, LV_ANIM_ON);
            lv_bar_set_start_value(sr_bar[i], sr_val[i] > 20 ? -sr_val[i] : -20, LV_ANIM_ON);
            lv_bar_set_start_value(sr_bar[7 - i], sr_val[i] > 20 ? -sr_val[i] : -20, LV_ANIM_ON);
        }
        sr_anim_count++;
    } else {
        /* The first timer callback will set the bars to 0 */
        if (sr_anim_count != 0) {
            for (size_t i = 0; i < 8; i++) {
                lv_bar_set_value(sr_bar[i], 0, LV_ANIM_ON);
                lv_bar_set_start_value(sr_bar[i], -0, LV_ANIM_ON);
            }
            sr_anim_count = 0;
        } else {
            /* The second timer callback will hide sr mask */
            if (!lv_obj_has_flag(sr_mask, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(sr_mask, LV_OBJ_FLAG_HIDDEN);
            }
        }
    } 
}

void ui_sr_anim_init(void)
{
    if (NULL == sr_mask) {
        sr_mask = lv_obj_create(lv_scr_act());
        lv_obj_set_size(sr_mask, 320, 210);
        lv_obj_clear_flag(sr_mask, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(sr_mask, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_radius(sr_mask, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(sr_mask, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(sr_mask, lv_obj_get_style_bg_color(lv_scr_act(), LV_PART_MAIN), LV_STATE_DEFAULT);
        lv_obj_align(sr_mask, LV_ALIGN_CENTER, 0, 15);
        lv_obj_add_event_cb(sr_mask, sr_mask_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }

    static lv_obj_t *obj_img = NULL;
    if (NULL == obj_img) {
        obj_img = lv_obj_create(sr_mask);
        lv_obj_set_size(obj_img, 80, 80);
        lv_obj_clear_flag(obj_img, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(obj_img, 40, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj_img, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(obj_img, 40, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(obj_img, LV_OPA_30, LV_STATE_DEFAULT);
        lv_obj_align(obj_img, LV_ALIGN_CENTER, 0, -30);
        lv_obj_t *img_mic_logo = lv_img_create(obj_img);
        LV_IMG_DECLARE(mic_logo)
        lv_img_set_src(img_mic_logo, &mic_logo);
        lv_obj_center(img_mic_logo);
    }

    if (NULL == sr_label) {
        LV_FONT_DECLARE(FONT_CMD)
        sr_label = lv_label_create(sr_mask);
        lv_label_set_text_static(sr_label, "");
        lv_obj_set_style_text_font(sr_label, &FONT_CMD, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(sr_label, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_align(sr_label, LV_ALIGN_CENTER, 0, 80);
        lv_obj_add_event_cb(sr_label, sr_label_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }

    for (size_t i = 0; i < sizeof(sr_bar) / sizeof(sr_bar[0]); i++) {
        if (NULL == sr_bar[i]) {
            sr_bar[i] = lv_bar_create(sr_mask);
            lv_obj_set_size(sr_bar[i], 5, 60);
            lv_obj_set_style_anim_time(sr_bar[i], 400, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(sr_bar[i], lv_color_make(237, 238, 239), LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(sr_bar[i], lv_color_make(246, 175, 171), LV_PART_INDICATOR);
            lv_bar_set_range(sr_bar[i], -100, 100);
            lv_bar_set_value(sr_bar[i], 20, LV_ANIM_OFF);
            lv_bar_set_start_value(sr_bar[i], -20, LV_ANIM_OFF);
            lv_obj_set_style_anim_time(sr_bar[i], 400, LV_STATE_DEFAULT);
        }
    }

    for (size_t i = 0; i < sizeof(sr_bar) / sizeof(sr_bar[0]) / 2; i++) {
        lv_obj_align_to(sr_bar[i], obj_img, LV_ALIGN_OUT_LEFT_MID, 15 * i - 65, 0);
        lv_obj_align_to(sr_bar[i + 4], obj_img, LV_ALIGN_OUT_RIGHT_MID, 15 * i + 20, 0);
    }

    sr_anim_count = 0;
    sr_anim_active = false;
    static lv_timer_t *timer = NULL;
    if (NULL == timer) {
        timer = lv_timer_create(ui_speech_anim_cb, 500, NULL);
    }
}

void sr_anim_start(void)
{
    lv_event_send(sr_mask, LV_EVENT_VALUE_CHANGED, (void *) true);
}

void sr_anim_stop(void)
{
    lv_event_send(sr_mask, LV_EVENT_VALUE_CHANGED, (void *) false);
}

void sr_anim_set_text(char *text)
{
    lv_event_send(sr_label, LV_EVENT_VALUE_CHANGED, (void *) text);
}
