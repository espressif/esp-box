/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "lvgl.h"
#include "app_wifi.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "ui_main.h"
#include "app_rmaker.h"
#include "app_pump.h"
#include "app_humidity.h"
#include "ui_sr.h"
#include "ui_net_config.h"
#include "ui_boot_animate.h"

#include "bsp_board.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "ui_main";

LV_FONT_DECLARE(font_en_16)

typedef struct {
    bool need_hint;
} ui_param_t;

#define MAIN_UI_STATUS_BAR_H (36)
static lv_group_t *g_btn_op_group = NULL;
static button_style_t g_btn_styles;

static lv_obj_t *g_label_name = NULL;
static lv_obj_t *g_lab_time = NULL;
static lv_obj_t *g_lab_wifi = NULL;
static lv_obj_t *g_status_bar = NULL;

static void ui_arc_animation_ctrl(int start);

static void ui_button_style_init(void)
{
    /*Init the style for the default state*/

    lv_style_init(&g_btn_styles.style);

    lv_style_set_radius(&g_btn_styles.style, 5);
    lv_style_set_bg_color(&g_btn_styles.style, lv_color_make(255, 255, 255));

    lv_style_set_border_opa(&g_btn_styles.style, LV_OPA_30);
    lv_style_set_border_width(&g_btn_styles.style, 2);
    lv_style_set_border_color(&g_btn_styles.style, lv_palette_main(LV_PALETTE_GREY));

    lv_style_set_shadow_width(&g_btn_styles.style, 7);
    lv_style_set_shadow_color(&g_btn_styles.style, lv_color_make(0, 0, 0));
    lv_style_set_shadow_ofs_x(&g_btn_styles.style, 0);
    lv_style_set_shadow_ofs_y(&g_btn_styles.style, 0);

    /*Init the pressed style*/

    lv_style_init(&g_btn_styles.style_pr);

    lv_style_set_border_opa(&g_btn_styles.style_pr, LV_OPA_40);
    lv_style_set_border_width(&g_btn_styles.style_pr, 2);
    lv_style_set_border_color(&g_btn_styles.style_pr, lv_palette_main(LV_PALETTE_GREY));


    lv_style_init(&g_btn_styles.style_focus);
    lv_style_set_outline_color(&g_btn_styles.style_focus, lv_color_make(255, 0, 0));

    lv_style_init(&g_btn_styles.style_focus_no_outline);
    lv_style_set_outline_width(&g_btn_styles.style_focus_no_outline, 0);
}

button_style_t *ui_button_styles(void)
{
    return &g_btn_styles;
}

lv_group_t *ui_get_btn_op_group(void)
{
    return g_btn_op_group;
}

static void ui_status_bar_set_visible(bool visible)
{
    if (visible) {
        lv_obj_clear_flag(g_status_bar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_status_bar, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_main_get_status_bar(void)
{
    return g_status_bar;
}

void ui_main_status_bar_set_wifi(bool is_connected)
{
    lv_label_set_text_static(g_lab_wifi, is_connected ? LV_SYMBOL_WIFI : "--");
}

typedef struct {
    char *name;
    void *img_src;
} item_desc_t;

LV_IMG_DECLARE(img_about_us_src)
LV_IMG_DECLARE(img_network_src)

static lv_timer_t *g_arc_timer = NULL;

static lv_obj_t *g_btn_item = NULL;
static lv_obj_t *g_btn_label_item = NULL;
static lv_obj_t *g_btn_arc_item = NULL;

static void btn_watering_handler(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_RELEASED) {
        if (app_pump_is_watering()) {
            app_pump_watering_stop();
        } else {
            app_pump_watering_start();
        }
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        if (app_pump_is_watering()) {
            //
            lv_obj_set_style_bg_color(btn, lv_color_white(), LV_PART_MAIN);
            int remaining_time = app_pump_watering_remaining_time();
            lv_label_set_text_fmt(label, "Watering\n   %02d:%02d", remaining_time / 60, remaining_time % 60);
            lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
            ui_arc_animation_ctrl(1);

        } else {
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
            lv_label_set_text(label, "OFF");
            lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
            ui_arc_animation_ctrl(0);
        }
    }
}



static void arc_watering_handler(lv_event_t *e)
{
    lv_obj_t *arc = lv_event_get_target(e);
    lv_timer_t *timer = lv_event_get_user_data(e);
    int16_t v = lv_arc_get_value(arc);
    v++;
    if (v == lv_arc_get_max_value(arc)) {
        //lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
        lv_timer_pause(timer);
    } else {
        lv_arc_set_value(arc, v);
    }
}

static void arc_timer_handler(lv_timer_t *param)
{
    lv_obj_t *arc = param->user_data;
    lv_event_send(arc, LV_EVENT_VALUE_CHANGED, NULL);
}

static void ui_arc_animation_ctrl(int start)
{
    vTaskDelay(pdMS_TO_TICKS(LV_DISP_DEF_REFR_PERIOD));
    if (start) {
        lv_arc_set_value(g_btn_arc_item, 0);
        lv_obj_clear_flag(g_btn_arc_item, LV_OBJ_FLAG_HIDDEN);

        lv_timer_reset(g_arc_timer);
        int period = app_pump_get_watering_time() * 1000 / lv_arc_get_max_value(g_btn_arc_item);
        lv_timer_set_period(g_arc_timer, period);
        lv_timer_resume(g_arc_timer);
    } else {
        lv_timer_pause(g_arc_timer);
        lv_obj_add_flag(g_btn_arc_item, LV_OBJ_FLAG_HIDDEN);
    }
}

static void ui_main_watering_before(void *args)
{
    lv_obj_set_style_bg_color(g_btn_item, lv_color_white(), LV_PART_MAIN);
    int remaining_time = app_pump_watering_remaining_time();
    lv_label_set_text_fmt(g_btn_label_item, "Watering\n   %02d:%02d", remaining_time / 60, remaining_time % 60);
    lv_obj_set_style_text_color(g_btn_label_item, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    ui_arc_animation_ctrl(1);
}

static void ui_main_watering_during(void *args)
{
    int remaining_time = app_pump_watering_remaining_time();
    lv_label_set_text_fmt(g_btn_label_item, "Watering\n   %02d:%02d", remaining_time / 60, remaining_time % 60);
}

static void ui_main_watering_after(void *args)
{
    lv_obj_set_style_bg_color(g_btn_item, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_label_set_text(g_btn_label_item, "OFF");
    lv_obj_set_style_text_color(g_btn_label_item, lv_color_white(), LV_PART_MAIN);
    ui_arc_animation_ctrl(0);
}

static void ui_watering_btn(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, 130, 130);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
    lv_obj_add_style(obj, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(obj, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_align(obj, LV_ALIGN_RIGHT_MID, 0, 0);

    g_btn_item = lv_btn_create(obj);
    lv_obj_t *btn = g_btn_item;
    lv_obj_set_size(btn, 120, 120);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_add_style(btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_center(btn);
    lv_obj_set_style_radius(btn, LV_STYLE_RADIUS, LV_STATE_DEFAULT);

    g_btn_label_item = lv_label_create(btn);
    lv_obj_t *label_btn = g_btn_label_item;
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_label_set_text(label_btn, "OFF");
    lv_obj_center(label_btn);

    lv_obj_add_event_cb(btn, btn_watering_handler, LV_EVENT_RELEASED, label_btn);
    lv_obj_add_event_cb(btn, btn_watering_handler, LV_EVENT_VALUE_CHANGED, label_btn);

#if CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn);
    }
#endif
    app_pump_add_cb_before_watering(ui_main_watering_before, NULL);
    app_pump_add_cb_during_watering(ui_main_watering_during, NULL);
    app_pump_add_cb_after_watering(ui_main_watering_after, NULL);
    //arc TODO
    g_btn_arc_item = lv_arc_create(obj);
    lv_obj_t *arc = g_btn_arc_item;
    lv_obj_set_size(arc, 120, 120);
    lv_arc_set_range(arc, 0, 500);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_obj_set_style_border_width(arc, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(arc, 0, LV_PART_MAIN);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(arc);
    //arc animation
    lv_arc_set_value(arc, 0);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
    g_arc_timer = lv_timer_create(arc_timer_handler, LV_DISP_DEF_REFR_PERIOD, arc);
    lv_timer_pause(g_arc_timer);
    lv_obj_add_event_cb(arc, arc_watering_handler, LV_EVENT_VALUE_CHANGED, g_arc_timer);
}

static void label_humidity_event_send(void *data)
{
    lv_obj_t *label = (lv_obj_t *) data;
    lv_event_send(label, LV_EVENT_VALUE_CHANGED, label);
}

static void label_humidity_value_handler(lv_event_t *e)
{
    lv_obj_t *label = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_label_set_text_fmt(label, "   %2d%%", app_humidity_get_value());
    }
}

static void label_time_handler(lv_timer_t *timer)
{
    char time_str[8];
    time_t time_val;
    lv_obj_t *label = timer->user_data;
    time(&time_val);
    struct tm time;
    localtime_r(&time_val, &time);

    sprintf(time_str, "%02d:%02d", time.tm_hour, time.tm_min);
    lv_label_set_text(label, time_str);
}

void ui_watering_update_device_name(const char *name)
{
    lv_label_set_text(g_label_name, name);
}

static void ui_watering_main(void)
{
    lv_obj_t *page = lv_obj_create(lv_scr_act());

    lv_obj_set_size(page, lv_obj_get_width(lv_scr_act()), lv_obj_get_height(lv_scr_act()) - MAIN_UI_STATUS_BAR_H);
    lv_obj_set_style_radius(page, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(page, 0, LV_PART_MAIN);
    lv_obj_align(page, LV_ALIGN_TOP_MID, 0, MAIN_UI_STATUS_BAR_H);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(237, 238, 239), LV_STATE_DEFAULT);

    //device name label
    g_label_name = lv_label_create(page);
    lv_obj_t *label_name = g_label_name;
    lv_obj_set_style_text_font(label_name, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(label_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_name, "Device Name");
    lv_obj_align(label_name, LV_ALIGN_LEFT_MID, 0, -40);

    //a red split line
    static lv_point_t line_points[] = {{0, 50}, {60, 50}};
    lv_obj_t *line = lv_line_create(page);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, 4, LV_PART_MAIN);
    lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(line, false, LV_PART_MAIN);
    lv_obj_align(line, LV_ALIGN_LEFT_MID, 0, -40);

    //rh label title
    lv_obj_t *label_rh_title = lv_label_create(page);
    lv_obj_set_style_text_font(label_rh_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(label_rh_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_rh_title, "Current RH");
    lv_obj_align(label_rh_title, LV_ALIGN_LEFT_MID, 0, 10);

    //rh label value
    lv_obj_t *label_rh_value = lv_label_create(page);
    lv_obj_set_style_text_font(label_rh_value, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(label_rh_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_rh_value, "   45%");
    lv_obj_align(label_rh_value, LV_ALIGN_LEFT_MID, 0, 40);
    lv_obj_add_event_cb(label_rh_value, label_humidity_value_handler, LV_EVENT_VALUE_CHANGED, label_rh_value);
    app_humidity_add_watcher(label_humidity_event_send, label_rh_value);
    ui_watering_btn(page);
}

static void ui_after_boot(void)
{
    ui_status_bar_set_visible(1);
    ui_watering_main();
}

void ui_main(void)
{
    bsp_display_lock(0);

    ui_button_style_init();
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(237, 238, 239), LV_STATE_DEFAULT);

    lv_indev_t *indev = lv_indev_get_next(NULL);

    if ((lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) || \
            lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
        ESP_LOGI(TAG, "Input device type is keypad");
        g_btn_op_group = lv_group_create();
        lv_indev_set_group(indev, g_btn_op_group);
    }

    // Create status bar
    g_status_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_status_bar, lv_obj_get_width(lv_obj_get_parent(g_status_bar)), MAIN_UI_STATUS_BAR_H);
    lv_obj_clear_flag(g_status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_status_bar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(g_status_bar, lv_color_make(200, 200, 200), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g_status_bar, 0, LV_PART_MAIN);
    lv_obj_align(g_status_bar, LV_ALIGN_TOP_LEFT, 0, 0);

    g_lab_time = lv_label_create(g_status_bar);
    lv_label_set_text(g_lab_time, "00:00");
    lv_obj_align(g_lab_time, LV_ALIGN_LEFT_MID, 0, 0);
    //static lv_timer_t *timer = lv_timer_create(label_time_handler, 1000, g_lab_time);
    lv_timer_create(label_time_handler, 1000, g_lab_time);

    g_lab_wifi = lv_label_create(g_status_bar);
    if (app_wifi_is_connected()) {
        ui_main_status_bar_set_wifi(1);
    } else {
        ui_main_status_bar_set_wifi(0);
    }
    lv_obj_align_to(g_lab_wifi, g_lab_time, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    // lv_timer_create(time_blink_cb, 500, (void *) g_lab_time);
    ui_status_bar_set_visible(0);

    /* For speech animation */
    ui_sr_anim_init();

    boot_animate_start(ui_after_boot);

    bsp_display_unlock();
}
