/**
 * @file ui_main.c
 * @brief 
 * @version 0.1
 * @date 2021-08-11
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
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "soc/soc_memory_layout.h"
#include "time.h"
#include "ui_main.h"

/* LVGL image declarations */
LV_IMG_DECLARE(bird)
LV_IMG_DECLARE(boot_logo)
LV_IMG_DECLARE(esp_box)
LV_IMG_DECLARE(logo)

/* LVGL font declarations */
LV_FONT_DECLARE(font_date_28)
LV_FONT_DECLARE(font_en_20)
LV_FONT_DECLARE(font_en_28)
LV_FONT_DECLARE(font_en_32)
LV_FONT_DECLARE(font_en_bold_72)
LV_FONT_DECLARE(font_symbol_20)
LV_FONT_DECLARE(font_temp_36)

typedef esp_err_t (*btn_back_call_back_t)(bool);

/* Forward declarations for private functions */
static void boot_animate_cb(lv_timer_t *timer);
static void time_blink_task(lv_timer_t *timer);
static esp_err_t ui_status_bar_init(bool show);
static esp_err_t ui_clock_start(bool show);

/* Command word handler */
extern void sr_task_handler(lv_timer_t *timer);

/* Private variable(s) */
static lv_obj_t *panel = NULL;
static lv_obj_t *bar_time = NULL;
static lv_obj_t *bar_wifi = NULL;
static btn_back_call_back_t btn_back_disp_cb = NULL;
static btn_back_call_back_t btn_back_hide_cb = NULL;

esp_err_t ui_main_start(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(244, 244, 244), 0);

    panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(panel, 280, 200);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(panel);

    lv_timer_create(boot_animate_cb, 80, (void *) panel);
    lv_timer_create(sr_task_handler, 100, (void *) panel);

    btn_back_hide_cb = ui_clock_start;

    return ESP_OK;
}

static esp_err_t ui_status_bar_init(bool show)
{
    static lv_obj_t *bar = NULL;

    if (NULL == bar) {
        bar = lv_obj_create(lv_scr_act());
        lv_obj_set_size(bar, 320, 30);
        lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(bar, lv_color_make(222, 230, 243), 0);
    }

    if (NULL == bar_time) {
        bar_time = lv_label_create(bar);
        lv_label_set_text(bar_time, "00:00");
        lv_obj_align(bar_time, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(bar_time, &font_en_20, 0);
        lv_obj_set_style_text_color(bar_time, lv_color_black(), 0);
    }

    if (NULL == bar_wifi) {
        bar_wifi = lv_label_create(bar);
        lv_label_set_text(bar_wifi, LV_SYMBOL_WIFI);
        lv_obj_align(bar_wifi, LV_ALIGN_RIGHT_MID, -20, 0);
        lv_obj_set_style_text_font(bar_wifi, &font_symbol_20, 0);
        lv_obj_set_style_text_color(bar_wifi, lv_color_black(), 0);
    }

    /* Also adjust size of panel */
    lv_obj_set_size(panel, 280, 170);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, -20);

    return ESP_OK;
}

static esp_err_t ui_clock_start(bool show)
{
    static lv_obj_t *img_bg = NULL;
    static lv_obj_t *label_date = NULL;
    static lv_obj_t *label_time = NULL;
    static lv_obj_t *label_temp = NULL;

    if (NULL == img_bg) {
        img_bg = lv_img_create(panel);
        lv_img_set_src(img_bg, &bird);
        lv_obj_center(img_bg);
    }

    if (NULL == label_time) {
        label_time = lv_label_create(panel);
        lv_label_set_text(label_time, "00:00");
        lv_obj_set_style_text_font(label_time, &font_en_bold_72, 0);
        lv_obj_align(label_time, LV_ALIGN_CENTER, 0, -30);
        lv_timer_create(time_blink_task, 500, (void *) label_time);
    }

    if (NULL == label_date) {
        label_date = lv_label_create(panel);
        lv_label_set_text(label_date, "8 月 24 日  星期二");
        lv_obj_set_style_text_font(label_date, &font_date_28, 0);
        lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 20);
    }

    if (NULL == label_temp) {
        label_temp = lv_label_create(panel);
        lv_label_set_text(label_temp, "25°C");
         lv_obj_set_style_text_font(label_temp, &font_temp_36, 0);
        lv_obj_align(label_temp, LV_ALIGN_CENTER, 0, 60);
    }

    /* Show or hide object */
    if (show) {
        lv_obj_clear_flag(img_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label_date, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label_time, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label_temp, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
        /* Set hide callback */
        btn_back_hide_cb = ui_clock_start;
    } else {
        lv_obj_add_flag(img_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label_date, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label_time, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label_temp, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
    }

    return ESP_OK;
}

static void boot_animate_cb(lv_timer_t *timer)
{
    static uint32_t count = 0;
    static lv_obj_t *img_logo = NULL;
    static lv_obj_t *img_text = NULL;
    static lv_obj_t *bar = NULL;
    static const int32_t exit_time_ms = 3000;
    
    count++;
    lv_obj_t *panel = (lv_obj_t *) timer->user_data;

    if (NULL == img_logo) {
        img_logo = lv_img_create(panel);
        lv_img_set_src(img_logo, &boot_logo);
        lv_obj_align(img_logo, LV_ALIGN_CENTER, 0, -40);
    }

    if (NULL == img_text) {
        img_text = lv_img_create(panel);
        lv_img_set_src(img_text, &esp_box);
        lv_obj_align(img_text, LV_ALIGN_CENTER, 0, 20);
    }

    if (NULL == bar) {
        bar = lv_bar_create(panel);
        lv_obj_set_size(bar, 200, 8);
        lv_bar_set_range(bar, 0, exit_time_ms / timer->period * 4 / 5);
        lv_obj_align_to(bar, img_text, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    }

    lv_bar_set_value(bar, count, LV_ANIM_ON);

    if (count > exit_time_ms / timer->period) {
        lv_obj_del(img_logo);
        lv_obj_del(img_text);
        lv_obj_del(bar);
        lv_timer_del(timer);
        ui_status_bar_init(true);
        ui_clock_start(true);
        btn_back_disp_cb = ui_clock_start;
        btn_back_hide_cb = ui_clock_start;
    }
}

static void time_blink_task(lv_timer_t *timer)
{
    static char fmt_text[8];
    static time_t time_utc;
    static struct tm time_local;
    static uint8_t disp_flag = 0;

    /* Get label object from `user_data` */
    lv_obj_t *label_time = (lv_obj_t *) timer->user_data;

    disp_flag = !disp_flag;

    time(&time_utc);
    time_utc += 13 * 60 * 60 + 20 * 60;

    localtime_r(&time_utc, &time_local);

    if (disp_flag) {
        sprintf(fmt_text, "%02d:%02d", time_local.tm_hour, time_local.tm_min);
    } else {
        sprintf(fmt_text, "%02d %02d", time_local.tm_hour, time_local.tm_min);
    }

    lv_label_set_text_static(label_time, fmt_text);
    lv_label_set_text_static(bar_time, fmt_text);
}
