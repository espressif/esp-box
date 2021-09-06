/**
 * @file ui_main.c
 * @brief 
 * @version 0.1
 * @date 2021-09-19
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

#include <math.h>
#include <stdbool.h>
#include <time.h>
#include "app_led.h"
#include "bsp_codec.h"
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lv_qrcode.h"
#include "ui_main.h"

#ifndef PI
#define PI  (3.14159f)
#endif

static const char *TAG = "ui_main";

/* **************** DECLARATION **************** */
LV_FONT_DECLARE(font_app)
LV_FONT_DECLARE(font_cmd_36)
LV_FONT_DECLARE(font_en_16)
LV_FONT_DECLARE(font_en_24)
LV_FONT_DECLARE(font_en_64)
LV_FONT_DECLARE(font_en_72)

LV_IMG_DECLARE(esp_logo_tiny)
LV_IMG_DECLARE(mute_on)
LV_IMG_DECLARE(mute_off)
LV_IMG_DECLARE(fan_on)
LV_IMG_DECLARE(fan_off)
LV_IMG_DECLARE(light_on)
LV_IMG_DECLARE(light_off)
LV_IMG_DECLARE(media_on)
LV_IMG_DECLARE(media_off)
LV_IMG_DECLARE(security_on)
LV_IMG_DECLARE(security_off)

static void boot_animate_start(lv_obj_t *scr);
static void ui_mute_init(void);
static void ui_status_bar_init(lv_color_t color);

static void ui_led_show(bool show);
static void ui_main_show(bool show);
static void ui_dev_ctrl_show(bool show);
static void ui_network_show(bool show);

/* **************** UI ENTRY **************** */
esp_err_t ui_main_start(void)
{
    if (NULL == lv_scr_act()) {
        ESP_LOGE(TAG, "LVGL not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ui_mute_init();

    boot_animate_start(lv_scr_act());

    return ESP_OK;
}

/* **************** MAIN UI **************** */
static lv_obj_t *label_clock = NULL;
static lv_obj_t *label_wifi = NULL;
static lv_obj_t *label_time = NULL;

static void clock_blink_cb(lv_timer_t *timer)
{
    static time_t time_val;
    static bool disp = true;
    static char time_str[8];

    time(&time_val);
    struct tm time;
    localtime_r(&time_val, &time);

    disp = !disp;
    if (disp) {
        sprintf(time_str, "%02d:%02d", time.tm_hour, time.tm_min);
    } else {
        sprintf(time_str, "%02d %02d", time.tm_hour, time.tm_min);
    }

    lv_label_set_text_static(label_time, time_str);
    lv_label_set_text_static(label_clock, time_str);
}

static void hint_close_cb(lv_event_t *event)
{
    lv_obj_t *obj = (lv_obj_t *) event->user_data;

    lv_obj_del(obj);
}

static void ui_hint_show(void)
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
        lv_label_set_text(label_title, "Steps for usage");
        lv_obj_set_style_text_color(label_title, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_title, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t *label_hint = lv_label_create(obj);
        lv_label_set_text(label_hint,
            "1: Say \"Hi ESP\" to wake-up the device.\n"
            "2: Wait for the \"Hi ESP\" shows on screen.\n"
            "3: Say command, like \"turn on the light\"");
        lv_obj_set_style_text_line_space(label_hint, 6, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_hint, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_hint, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_align(label_hint, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *label_close = lv_label_create(obj);
        lv_label_set_text(label_close, "OK Let's Go");
        lv_obj_set_ext_click_area(label_close, 20);
        lv_obj_add_flag(label_close, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(label_close, lv_color_make(251, 17, 26), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_close, lv_color_black(), LV_STATE_PRESSED);
        lv_obj_set_style_text_font(label_close, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_align(label_close, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_event_cb(label_close, hint_close_cb, LV_EVENT_CLICKED, (void *) obj);
    }
}

static void ui_main_show(bool show)
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
    }

    if (NULL == label_time) {
        label_time = lv_label_create(panel);
        lv_label_set_text(label_time, "16:44");
        lv_obj_set_style_text_font(label_time, &font_en_64, 0);
        lv_obj_set_style_text_color(label_time, lv_color_make(40, 40, 40), 0);
        lv_obj_align(label_time, LV_ALIGN_CENTER, 0, -20);
    }

    static lv_obj_t *label_date = NULL;
    if (NULL == label_date) {
        label_date = lv_label_create(panel);
        lv_label_set_text(label_date, "Thursday, October 14");
        lv_obj_set_style_text_font(label_date, &font_en_24, 0);
        lv_obj_set_style_text_color(label_date, lv_color_make(80, 80, 80), 0);
        lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 30);
    }

    if (show) {
        ui_status_bar_init(lv_color_make(80, 80, 80));
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
    }
}

/* **************** STATUS BAR **************** */
static void btn_app_cb(lv_event_t *event)
{
    (void) event;

    ui_main_show(false);
    ui_network_show(false);
    ui_led_show(false);

    ui_dev_ctrl_show(true);
}

static void btn_network_cb(lv_event_t *event)
{
    ui_main_show(false);
    ui_dev_ctrl_show(false);
    ui_led_show(false);
    ui_network_show(true);
}

static void btn_clock_cb(lv_event_t *event)
{
    // hide_current_scene();
    ui_main_show(false);
    ui_network_show(false);
    ui_dev_ctrl_show(false);
    ui_led_show(false);

    ui_main_show(true);
}

static void ui_status_bar_init(lv_color_t color)
{
    if (NULL == label_clock) {
        label_clock = lv_label_create(lv_scr_act());
        lv_obj_add_flag(label_clock, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(label_clock, "08:00");
        lv_obj_set_style_text_font(label_clock, &font_en_16, 0);
        lv_obj_set_style_text_color(label_clock, color, 0);
        lv_obj_align(label_clock, LV_ALIGN_TOP_LEFT, 10, 5);
        lv_obj_add_event_cb(label_clock, btn_clock_cb, LV_EVENT_CLICKED, NULL);
    }

    if (NULL == label_wifi) {
        label_wifi = lv_label_create(lv_scr_act());
        lv_obj_add_flag(label_wifi, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(label_wifi, LV_SYMBOL_WIFI);
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
        lv_label_set_text(btn_app, "\xef\xa8\x80");
        lv_obj_align(btn_app, LV_ALIGN_TOP_RIGHT, -40, 7);
        lv_obj_add_event_cb(btn_app, btn_app_cb, LV_EVENT_CLICKED, NULL);
    }

    static lv_obj_t *btn_setting = NULL;
    if (NULL == btn_setting) {
        btn_setting = lv_label_create(lv_scr_act());
        lv_obj_add_flag(btn_setting, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_font(btn_setting, &lv_font_montserrat_16, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_setting, color, 0);
        lv_label_set_text(btn_setting, LV_SYMBOL_SETTINGS);
        lv_obj_align(btn_setting, LV_ALIGN_TOP_RIGHT, -10, 5);
    }

    lv_obj_set_style_text_color(label_clock, color, 0);
    lv_obj_set_style_text_color(label_wifi, color, 0);
    lv_obj_set_style_text_color(btn_app, color, 0);
    lv_obj_set_style_text_color(btn_setting, color, 0);

}

/* **************** DEVICE CONTROL **************** */
typedef struct {
    const char *name;
    lv_img_dsc_t const* img_on;
    lv_img_dsc_t const* img_off;
} btn_img_src_t;

static lv_obj_t *obj_dev[4] = { [0 ... 3] = NULL };

static void btn_dev_cb(lv_event_t *event)
{
    lv_obj_t *btn = (lv_obj_t *) event->target;
    lv_obj_t *img = (lv_obj_t *) event->user_data;
    lv_obj_t *lab = (lv_obj_t *) btn->user_data;
    btn_img_src_t *p_img_src = (btn_img_src_t *) img->user_data;

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

    if (btn == obj_dev[0]) {
        if (btn->state & LV_STATE_CHECKED) {
            app_led_set_all(30, 30, 30);
        } else {
            app_led_set_all(0, 0, 0);
        }
    }
}

static void btn_dev_detail_cb(lv_event_t *event)
{
    int32_t dev_id = (int32_t) event->user_data;

    if (dev_id == 0) {
        ui_dev_ctrl_show(false);
        ui_led_show(true);
    }
}

static void btn_back_dev_ctrl_cb(lv_event_t *event)
{
    ui_dev_ctrl_show(false);

    ui_main_show(true);
}

static void ui_dev_ctrl_show(bool show)
{
    static lv_obj_t *btn_back = NULL;
    static const btn_img_src_t img_src_list[] = {
        { .name = "Light", .img_on = &light_on, .img_off = &light_off },
        { .name = "Meida", .img_on = &media_on, .img_off = &media_off },
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
            lv_obj_add_event_cb(obj_dev[i], btn_dev_cb, LV_EVENT_CLICKED, (void *) img);
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
        ui_status_bar_init(lv_color_make(80, 80, 80));
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(237, 238, 239), LV_STATE_DEFAULT);
        lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < 4; i++) {
            lv_obj_clear_flag(obj_dev[i], LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(btn_back, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < 4; i++) {
            lv_obj_add_flag(obj_dev[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* **************** LED CONTROL **************** */
static void btn_color_cb(lv_event_t *event)
{
    lv_obj_t *color_sel = (lv_obj_t *) event->user_data;
    lv_obj_t *color_btn = (lv_obj_t *) event->target;
    lv_color_t color = lv_obj_get_style_bg_color(color_btn, LV_PART_MAIN);

    lv_colorwheel_set_rgb(color_sel, color);
}

static void btn_back_led_ctrl_cb(lv_event_t *event)
{
    (void) event;

    ui_led_show(false);
    ui_dev_ctrl_show(true);
}

static void ui_led_show(bool show)
{
    static lv_obj_t *color_sel = NULL;
    if (NULL == color_sel) {
        color_sel = lv_colorwheel_create(lv_scr_act(), true);
        lv_obj_set_size(color_sel, 128, 128);
        lv_obj_set_style_arc_width(color_sel, 15, LV_STATE_DEFAULT);
        lv_obj_align(color_sel, LV_ALIGN_CENTER, 0, -20);
    }

    static lv_obj_t *btn[4] = { [0 ... 3] = NULL };
    static lv_color_t color_list[] = {
        LV_COLOR_MAKE(210, 50, 100), //LV_COLOR_MAKE(204, 204, 204),
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
            lv_obj_add_event_cb(btn[i], btn_color_cb, LV_EVENT_CLICKED, (void *) color_sel);
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

/* **************** NETWORK PAGE **************** */
static void btn_back_network_cb(lv_event_t *event)
{
    ui_network_show(false);
    ui_main_show(true);
}

static void ui_network_show(bool show)
{
    static lv_obj_t *mask = NULL;

    if (NULL == mask) {
        mask = lv_obj_create(lv_scr_act());
        lv_obj_set_style_radius(mask, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(mask, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(mask, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_size(mask, 290, 180);
        lv_obj_align(mask, LV_ALIGN_BOTTOM_MID, 0, -15);
        // lv_obj_clear_flag(mask, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }

    static lv_obj_t *btn_back = NULL;
    if (NULL == btn_back) {
        btn_back = lv_label_create(mask);
        lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(btn_back, 15);
        lv_label_set_text_static(btn_back, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_font(btn_back, &lv_font_montserrat_16, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_back, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(btn_back, lv_color_white(), LV_STATE_PRESSED);
        lv_obj_align(btn_back, LV_ALIGN_CENTER, -120, -48 - 20);
        lv_obj_add_event_cb(btn_back, btn_back_network_cb, LV_EVENT_CLICKED, NULL);
    }

    static lv_obj_t *label = NULL;
    if (NULL == label) {
        label = lv_label_create(mask);
        lv_label_set_text_static(label, "Scan the QRcode to connect");
        lv_obj_set_style_text_font(label, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -48 - 20);
    }

    static lv_obj_t *qr = NULL;
    const char * qr_data = "WIFI:S:ESP_BOX;P:12345678;";
    if (NULL == qr) {
        lv_obj_t * qr = lv_qrcode_create(mask, 92, lv_color_black(), lv_color_white());
        lv_qrcode_update(qr, qr_data, strlen(qr_data));
        lv_obj_align(qr, LV_ALIGN_CENTER, 0, 5);

        lv_obj_t *img = lv_img_create(qr);
        lv_img_set_src(img, &esp_logo_tiny);
        lv_obj_center(img);
    }

    static lv_obj_t *lab_net_state = NULL;
    if (NULL == lab_net_state) {
        lab_net_state = lv_label_create(mask);
        lv_label_set_text(lab_net_state, "Not connected");
        lv_obj_set_style_text_font(lab_net_state, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_align(lab_net_state, LV_ALIGN_BOTTOM_MID, 0, 0);
    }

    if (show) {
        lv_obj_clear_flag(mask, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mask, LV_OBJ_FLAG_HIDDEN);
    }
}

/* **************** MUTE INDICATOR **************** */
static int32_t mute_disp_count = 15;
static const int32_t disp_time = 15;  
static bool mute_state = false;  

static void mute_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *obj = (lv_obj_t *) timer->user_data;
    lv_obj_t *img = (lv_obj_t *) obj->user_data;

    if (mute_disp_count <= disp_time) {
        if (mute_disp_count == 0) {
            if (mute_state) {
                lv_img_set_src(img, &mute_on);
            } else {
                lv_img_set_src(img, &mute_off);
            }
        } else if (disp_time != mute_disp_count) {
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(obj);
        } else {
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        mute_disp_count++;
    }
}

static void ui_mute_init(void)
{
    static lv_obj_t *obj = NULL;
    static lv_obj_t *img = NULL;
    if (NULL == obj) {
        obj = lv_obj_create(lv_scr_act());
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(obj, 180, 45);
        lv_obj_set_style_bg_color(obj, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(obj, 10, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(obj, 20, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(obj, LV_OPA_50, LV_STATE_DEFAULT);
        lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 10);

        lv_obj_t *lab = lv_label_create(obj);
        lv_label_set_text_static(lab, "Voice Command");
        lv_obj_set_style_text_font(lab, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lab, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_align(lab, LV_ALIGN_CENTER, 10, 0);

        img = lv_img_create(obj);
        lv_img_set_src(img, &mute_on);
        lv_obj_align(img, LV_ALIGN_CENTER, -70, 0); 
        lv_obj_set_user_data(obj, (void *) img);
    }

    static lv_timer_t *timer = NULL;
    if (NULL == timer) {
        timer = lv_timer_create(mute_timer_cb, 100, (void *) obj);
    }
}

void ui_mute_set_state(bool mute)
{
    mute_state = mute;
    mute_disp_count = 0;
}

/**
 * @brief Mute button handler.
 * 
 * @note Due to other examples might have no handler of mute button. So the
 *       default handler just simply print the mute button state and marked
 *       as `WEAK` so that you can rewrite the isr handler.
 * 
 * @param arg Unused
 */
void gpio_isr_handler(void *arg)
{
    (void) arg;

    if (gpio_get_level(GPIO_NUM_1)) {
        ui_mute_set_state(false);
    } else {
        ui_mute_set_state(true);
    }
}

/* **************** SPEECH RECOGNITION ANIMATE **************** */
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

    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 80);
}

/* **************** BOOT ANIMATE **************** */
LV_IMG_DECLARE(esp_logo)
LV_IMG_DECLARE(esp_text)

static lv_obj_t *arc[3];
static lv_obj_t *img_logo;
static lv_obj_t *img_text;
static lv_color_t arc_color[] = {
    LV_COLOR_MAKE(232, 87, 116),
    LV_COLOR_MAKE(126, 87, 162),
    LV_COLOR_MAKE(90, 202, 228),
};

static void anim_timer_cb(lv_timer_t *timer)
{
    static int32_t count = -90;
    lv_obj_t *img_logo = (lv_obj_t *) timer->user_data;

    if (count < 90) {
        lv_coord_t arc_start = count > 0 ? (1 - cosf(count / 180.0f * PI)) * 270: 0;
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

        img_text = lv_img_create(lv_obj_get_parent(img_logo));
        lv_img_set_src(img_text, &esp_text);
        lv_obj_set_style_img_opa(img_text, 0, 0);
    }

    if ((count >= 100) && (count <= 180)) {
        lv_coord_t offset = (sinf((count - 140) * 2.25f / 90.0f) + 1) * 20.0f;
        lv_obj_align(img_logo, LV_ALIGN_CENTER, 0, -offset);
        lv_obj_align(img_text, LV_ALIGN_CENTER, 0, 2 * offset);
        lv_obj_set_style_img_opa(img_text, offset / 40.0f * 255, 0);
    }

    if ((count += 2) >= 220) {
        lv_timer_del(timer);
        lv_obj_del(img_logo);
        lv_obj_del(img_text);

        /* Start speech recognition demo UI */
        ui_main_show(true);
        ui_hint_show();
        lv_timer_create(clock_blink_cb, 1000, NULL);
    }
}

static void boot_animate_start(lv_obj_t *scr)
{
    img_logo = lv_img_create(scr);
    lv_img_set_src(img_logo, &esp_logo);
    lv_obj_center(img_logo);

    for (size_t i = 0; i < sizeof(arc) / sizeof(arc[0]); i++) {
        arc[i] = lv_arc_create(scr);

        lv_obj_set_size(arc[i], 220 - 30 * i, 220 - 30 * i);
        lv_arc_set_bg_angles(arc[i], 120 * i, 10 + 120 * i);
        lv_arc_set_value(arc[i], 0);
        
        lv_obj_remove_style(arc[i], NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(arc[i], 10, 0);
        lv_obj_set_style_arc_color(arc[i], arc_color[i], 0);

        lv_obj_center(arc[i]);
    }

    lv_timer_create(anim_timer_cb, 20, (void *) img_logo);
}
