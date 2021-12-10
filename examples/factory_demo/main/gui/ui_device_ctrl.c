/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_log.h"
#include "bsp_board.h"
#include "lvgl/lvgl.h"
#include "app_led.h"
#include "app_fan.h"
#include "app_switch.h"
#include "ui_main.h"
#include "ui_device_ctrl.h"

static const char *TAG = "ui_dev_ctrl";

LV_FONT_DECLARE(font_en_16);

LV_IMG_DECLARE(icon_fan_on)
LV_IMG_DECLARE(icon_fan_off)
LV_IMG_DECLARE(icon_light_on)
LV_IMG_DECLARE(icon_light_off)
LV_IMG_DECLARE(icon_switch_on)
LV_IMG_DECLARE(icon_switch_off)
LV_IMG_DECLARE(icon_air_on)
LV_IMG_DECLARE(icon_air_off)

static ui_dev_type_t g_active_dev_type = UI_DEV_LIGHT;
static lv_obj_t *g_func_btn[4] = {NULL};
static void (*g_dev_ctrl_end_cb)(void) = NULL;

typedef struct {
    ui_dev_type_t type;
    const char *name;
    lv_img_dsc_t const *img_on;
    lv_img_dsc_t const *img_off;
} btn_img_src_t;

static const btn_img_src_t img_src_list[] = {
    { .type = UI_DEV_LIGHT, .name = "Light", .img_on = &icon_light_on, .img_off = &icon_light_off },
    { .type = UI_DEV_SWITCH, .name = "Switch", .img_on = &icon_switch_on, .img_off = &icon_switch_off },
    { .type = UI_DEV_FAN, .name = "Fan", .img_on = &icon_fan_on, .img_off = &icon_fan_off },
    { .type = UI_DEV_AIR, .name = "Air", .img_on = &icon_air_on, .img_off = &icon_air_off },
};

void ui_dev_ctrl_set_state(ui_dev_type_t type, bool state)
{
    if (NULL == g_func_btn[type]) {
        return;
    }
    ui_acquire();
    lv_obj_t *img = (lv_obj_t *) g_func_btn[type]->user_data;
    if (state) {
        lv_obj_add_state(g_func_btn[type], LV_STATE_CHECKED);
        lv_img_set_src(img, img_src_list[type].img_on);
    } else {
        lv_obj_clear_state(g_func_btn[type], LV_STATE_CHECKED);
        lv_img_set_src(img, img_src_list[type].img_off);
    }
    ui_release();
}

static void ui_dev_ctrl_page_func_click_cb(lv_event_t *e)
{
    uint32_t i = (uint32_t)lv_event_get_user_data(e);
    g_active_dev_type = i;

    if (UI_DEV_LIGHT == g_active_dev_type) {
        bool state = app_pwm_led_get_state();
        app_pwm_led_set_power(!state);
    } else if (UI_DEV_SWITCH == g_active_dev_type) {
        bool state = app_switch_get_state();
        app_switch_set_power(!state);
    } else if (UI_DEV_FAN == g_active_dev_type) {
        bool state = app_fan_get_state();
        app_fan_set_power(!state);
    } else {
        // Other device Not be supported, just update the UI
        ui_dev_ctrl_set_state(g_active_dev_type, !lv_obj_has_state(g_func_btn[g_active_dev_type], LV_STATE_CHECKED));
    }
}

static void ui_dev_ctrl_page_return_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    if (ui_get_btn_op_group()) {
        lv_group_remove_all_objs(ui_get_btn_op_group());
    }
    if (ui_get_button_indev()) {
        lv_indev_set_button_points(ui_get_button_indev(), NULL);
    }
    lv_obj_del(obj);
    g_func_btn[0] = NULL;
    g_func_btn[1] = NULL;
    g_func_btn[2] = NULL;
    g_func_btn[3] = NULL;
    if (g_dev_ctrl_end_cb) {
        g_dev_ctrl_end_cb();
    }
}

void ui_device_ctrl_start(void (*fn)(void))
{
    ESP_LOGI(TAG, "device control initialize");
    g_dev_ctrl_end_cb = fn;

    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, lv_obj_get_width(lv_obj_get_parent(page)), lv_obj_get_height(lv_obj_get_parent(page)) - lv_obj_get_height(ui_main_get_status_bar()));
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(page, lv_obj_get_style_bg_color(lv_scr_act(), LV_STATE_DEFAULT), LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(page, ui_main_get_status_bar(), LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    lv_obj_t *btn_return = lv_btn_create(page);
    lv_obj_set_size(btn_return, 24, 24);
    lv_obj_add_style(btn_return, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_align(btn_return, LV_ALIGN_TOP_LEFT, 0, -8);
    lv_obj_t *lab_btn_text = lv_label_create(btn_return);
    lv_label_set_text_static(lab_btn_text, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(lab_btn_text);
    lv_obj_add_event_cb(btn_return, ui_dev_ctrl_page_return_click_cb, LV_EVENT_CLICKED, page);

    for (size_t i = 0; i < 4; i++) {
        g_func_btn[i] = lv_btn_create(page);
        lv_obj_set_size(g_func_btn[i], 85, 85);
        lv_obj_add_style(g_func_btn[i], &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
        lv_obj_add_style(g_func_btn[i], &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(g_func_btn[i], lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(g_func_btn[i], lv_color_white(), LV_STATE_CHECKED);
        lv_obj_set_style_shadow_color(g_func_btn[i], lv_color_make(0, 0, 0), LV_PART_MAIN);
        lv_obj_set_style_shadow_width(g_func_btn[i], 10, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(g_func_btn[i], LV_OPA_40, LV_PART_MAIN);

        lv_obj_set_style_border_width(g_func_btn[i], 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_func_btn[i], lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);

        lv_obj_set_style_radius(g_func_btn[i], 10, LV_STATE_DEFAULT);
        lv_obj_align(g_func_btn[i], LV_ALIGN_CENTER, i % 2 ? 48 : -48, i < 2 ? -48 - 3 : 48 - 3);

        lv_obj_t *img = lv_img_create(g_func_btn[i]);
        lv_img_set_src(img, img_src_list[i].img_off);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, -10);
        lv_obj_set_user_data(img, (void *) &img_src_list[i]);

        lv_obj_t *label = lv_label_create(g_func_btn[i]);
        lv_label_set_text_static(label, img_src_list[i].name);
        lv_obj_set_style_text_color(label, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);

        lv_obj_set_user_data(g_func_btn[i], (void *) img);
        if (UI_DEV_LIGHT == i) {
            ui_dev_ctrl_set_state(i, app_pwm_led_get_state());
        } else if (UI_DEV_SWITCH == i) {
            ui_dev_ctrl_set_state(i, app_switch_get_state());
        } else if (UI_DEV_FAN == i) {
            ui_dev_ctrl_set_state(i, app_fan_get_state());
        }
        lv_obj_add_event_cb(g_func_btn[i], ui_dev_ctrl_page_func_click_cb, LV_EVENT_CLICKED, (void *)i);
        if (ui_get_btn_op_group()) {
            lv_group_add_obj(ui_get_btn_op_group(), g_func_btn[i]);
        }
    }
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_return);
    }
    if (ui_get_button_indev()) {
        lv_obj_update_layout(btn_return);
        lv_area_t a;
        lv_obj_get_click_area(btn_return, &a);
        static lv_point_t points_array[1];
        points_array[0].x = (a.x1 + a.x2) / 2;
        points_array[0].y = (a.y1 + a.y2) / 2;
        lv_indev_set_button_points(ui_get_button_indev(), points_array);
    }
}

