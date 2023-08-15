/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include <math.h>
#include "lvgl.h"
#include "ui_main.h"

#ifndef PI
#define PI  (3.14159f)
#endif

static lv_obj_t *arc[3];
static void (*g_boot_anim_end_cb)(void) = NULL;

static void anim_timer_cb(lv_timer_t *timer)
{
    static int32_t count = -90;
    lv_obj_t *page = (lv_obj_t *) timer->user_data;
    static lv_obj_t *img_logo = NULL;
    static lv_obj_t *img_text = NULL;

    if (-90 == count) {
        LV_IMG_DECLARE(esp_logo);
        img_logo = lv_img_create(page);
        lv_img_set_src(img_logo, &esp_logo);
        lv_obj_center(img_logo);
    }

    if (count < 90) {
        lv_coord_t arc_start = count > 0 ? (1 - cosf(count / 180.0f * PI)) * 270 : 0;
        lv_coord_t arc_len = (sinf(count / 180.0f * PI) + 1) * 135;

        for (size_t i = 0; i < sizeof(arc) / sizeof(arc[0]); i++) {
            lv_arc_set_bg_angles(arc[i], arc_start, arc_len);
            lv_arc_set_rotation(arc[i], (count + 120 * (i + 1)) % 360);
        }
    }

    if (count == 90) {
        for (size_t i = 0; i < sizeof(arc) / sizeof(arc[0]); i++) {
            lv_obj_del(arc[i]);
        }

        LV_IMG_DECLARE(esp_text);
        img_text = lv_img_create(page);
        lv_img_set_src(img_text, &esp_text);
        lv_obj_set_style_img_opa(img_text, 0, 0);
    }

    if ((count >= 100) && (count <= 180)) {
        lv_coord_t offset = (sinf((count - 140) * 2.25f / 90.0f) + 1) * 15.0f;
        lv_obj_align(img_logo, LV_ALIGN_CENTER, 0, -offset);
        lv_obj_align(img_text, LV_ALIGN_CENTER, 0, 2 * offset);
        lv_obj_set_style_img_opa(img_text, offset / 30.0f * 255, 0);
    }

    if ((count += 2) >= 300) {
        lv_timer_del(timer);
        lv_obj_del(page);

        if (g_boot_anim_end_cb) {
            g_boot_anim_end_cb();
        }
    }
}

void boot_animate_start(void (*fn)(void))
{
    g_boot_anim_end_cb = fn;

    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, lv_obj_get_width(lv_obj_get_parent(page)), lv_obj_get_height(lv_obj_get_parent(page)));
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(page, 0, LV_PART_MAIN);

    const lv_color_t arc_color[] = {
        LV_COLOR_MAKE(237, 228, 239),
        LV_COLOR_MAKE(211, 211, 211),
        LV_COLOR_MAKE(239, 218, 218),
    };
    for (size_t i = 0; i < sizeof(arc) / sizeof(arc[0]); i++) {
        arc[i] = lv_arc_create(page);
        lv_obj_set_size(arc[i], 220 - 30 * i, 220 - 30 * i);
        lv_arc_set_bg_angles(arc[i], 120 * i, 0 + 120 * i);
        lv_arc_set_value(arc[i], 0);
        lv_obj_remove_style(arc[i], NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(arc[i], 9, 0);
        lv_obj_set_style_arc_color(arc[i], arc_color[i], 0);
        lv_obj_center(arc[i]);
    }

    /* Create a timer to update time */
    lv_timer_create(anim_timer_cb, 20, page);
}
