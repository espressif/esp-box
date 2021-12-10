/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <wifi_provisioning/manager.h>
#include "esp_log.h"
#include "bsp_board.h"
#include "lvgl/lvgl.h"
#include "app_wifi.h"
#include "app_rmaker.h"
#include "ui_main.h"
#include "ui_net_config.h"

static const char *TAG = "ui_net_config";

static lv_obj_t *g_btn_app_hint = NULL;
static lv_obj_t *g_hint_lab = NULL;
static lv_obj_t *g_qr = NULL;
static lv_obj_t *g_img = NULL;
static lv_obj_t *g_page = NULL;
static ui_net_state_t g_net_state = UI_NET_EVT_LOARDING;

static void (*g_net_config_end_cb)(void) = NULL;

static void ui_app_page_return_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    if (ui_get_btn_op_group()) {
        lv_group_focus_freeze(ui_get_btn_op_group(), false);
    }
    if (ui_get_button_indev()) {
        lv_indev_set_button_points(ui_get_button_indev(), NULL);
    }
    lv_obj_del(obj);
}

static void ui_net_config_page_app_click_cb(lv_event_t *e)
{
    {
        /* **************** FRAMWORK **************** */
        lv_obj_t *page = lv_obj_create(lv_scr_act());
        lv_obj_set_size(page, lv_obj_get_width(lv_obj_get_parent(page)), 185);
        lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(page, 15, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(page, 1, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(page, 20, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(page, LV_OPA_30, LV_PART_MAIN);
        lv_obj_align(page, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *btn_return = lv_btn_create(page);
        lv_obj_set_size(btn_return, 24, 24);
        lv_obj_add_style(btn_return, &ui_button_styles()->style, 0);
        lv_obj_add_style(btn_return, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
        lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
        lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
        lv_obj_align(btn_return, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_t *lab_btn_text = lv_label_create(btn_return);
        lv_label_set_text_static(lab_btn_text, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
        lv_obj_center(lab_btn_text);
        lv_obj_add_event_cb(btn_return, ui_app_page_return_click_cb, LV_EVENT_CLICKED, page);
        if (ui_get_btn_op_group()) {
            lv_group_add_obj(ui_get_btn_op_group(), btn_return);
            lv_group_focus_obj(btn_return);
            lv_group_focus_freeze(ui_get_btn_op_group(), true);
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

        /* **************** HINT MESSAGE **************** */
        lv_obj_t *hint_label = lv_label_create(page);
        lv_label_set_text_static(hint_label,
                                 "Please scan the QR code below to\n"
                                 "download the ESP-BOX APP.");
        lv_obj_align(hint_label, LV_ALIGN_TOP_MID, 10, 0);

        /* **************** QR CODE **************** */
        static const char *qr_payload = "https://espressif.com/esp-box";
        lv_obj_t *qr = lv_qrcode_create(page, 92, lv_color_black(), lv_color_white());
        lv_qrcode_update(qr, qr_payload, strlen(qr_payload));
        lv_obj_align(qr, LV_ALIGN_CENTER, 0, 10);

        /* **************** LINK ADDR **************** */
        lv_obj_t *lab_link = lv_label_create(page);
        lv_label_set_text_static(lab_link, qr_payload);
        lv_obj_align(lab_link, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

static void ui_net_config_page_return_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    lv_obj_del(g_btn_app_hint);
    if (ui_get_btn_op_group()) {
        lv_group_remove_all_objs(ui_get_btn_op_group());
    }
    if (ui_get_button_indev()) {
        lv_indev_set_button_points(ui_get_button_indev(), NULL);
    }
    lv_obj_del(obj);
    g_page = NULL;
    if (g_net_config_end_cb) {
        g_net_config_end_cb();
    }
}

void ui_net_config_update_cb(ui_net_state_t state, void *args)
{
    g_net_state = state;
    if (!g_page) {
        return;
    }
    ui_acquire();
    switch (state) {
    case UI_NET_EVT_LOARDING:
        lv_obj_clear_flag(g_hint_lab, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(g_hint_lab, "System is loading ...");
        lv_obj_align(g_hint_lab, LV_ALIGN_CENTER, 0, 0);
        break;
    case UI_NET_EVT_START:
        /* code */
        break;
    case UI_NET_EVT_START_PROV:
        /* code */
        break;
    case UI_NET_EVT_GET_NAME: {
        LV_IMG_DECLARE(esp_logo_tiny);
        const char *prov_msg = app_wifi_get_prov_payload();
        size_t prov_msg_len = strlen(prov_msg);
        g_qr = lv_qrcode_create(g_page, 108, lv_color_black(), lv_color_white());
        ESP_LOGI(TAG, "QR Data: %s", prov_msg);
        char *p = strstr(prov_msg, "\"name\":\"");
        if (p) {
            p += 8;
            char *p_end = strstr(p, "\"");
            if (p_end) {
                char name[32] = {0};
                strncpy(name, p, p_end - p);
                lv_obj_t *lab_name = lv_label_create(g_page);
                lv_label_set_text(lab_name, name);
                lv_obj_align_to(lab_name, g_page, LV_ALIGN_TOP_MID, 0, -8);
            }
        }
        lv_obj_align(g_qr, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_t *img = lv_img_create(g_qr);
        lv_img_set_src(img, &esp_logo_tiny);
        lv_obj_center(img);
        lv_qrcode_update(g_qr, prov_msg, prov_msg_len);
        lv_obj_clear_flag(g_hint_lab, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(g_hint_lab,
                          "1.Open ESP-BOX APP on your phone\n"
                          "2.Scan the QR Code to provision");
        lv_obj_align_to(g_hint_lab, g_qr, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }
    break;
    case UI_NET_EVT_START_CONNECT:
        lv_obj_clear_flag(g_hint_lab, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(g_hint_lab, "Connecting to Wi-Fi ...");
        break;
    case UI_NET_EVT_WIFI_CONNECTED: {
        if (g_qr) {
            lv_obj_add_flag(g_qr, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_clear_flag(g_hint_lab, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(g_hint_lab, "Connecting to Rainmaker ...");
        lv_obj_align(g_hint_lab, LV_ALIGN_CENTER, 0, 0);
    }
    break;
    case UI_NET_EVT_CLOUD_CONNECTED: {
        char ssid[64] = {0};
        app_wifi_get_wifi_ssid(ssid, sizeof(ssid));
        LV_IMG_DECLARE(icon_rmaker);
        g_img = lv_img_create(g_page);
        lv_img_set_src(g_img, &icon_rmaker);
        lv_obj_align(g_img, LV_ALIGN_CENTER, 0, -10);
        lv_obj_clear_flag(g_hint_lab, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(g_hint_lab, "Device already connected to cloud\n"
                              "Wi-Fi is connected to #000000 %s#", ssid);
        lv_obj_align_to(g_hint_lab, g_img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }
    break;
    default:
        break;
    }
    ui_release();
}

void ui_net_config_start(void (*fn)(void))
{
    g_net_config_end_cb = fn;

    g_page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_page, 290, 190);
    lv_obj_clear_flag(g_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_page, 15, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(g_page, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(g_page, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_page, LV_OPA_30, LV_PART_MAIN);
    lv_obj_align(g_page, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *btn_return = lv_btn_create(g_page);
    lv_obj_set_size(btn_return, 24, 24);
    lv_obj_add_style(btn_return, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_align(btn_return, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *lab_btn_text = lv_label_create(btn_return);
    lv_label_set_text_static(lab_btn_text, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(lab_btn_text);
    lv_obj_add_event_cb(btn_return, ui_net_config_page_return_click_cb, LV_EVENT_CLICKED, g_page);
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

    /* **************** APP NOT INSTALLED **************** */
    g_btn_app_hint = lv_btn_create(ui_main_get_status_bar());
    lv_obj_set_size(g_btn_app_hint, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(g_btn_app_hint, &ui_button_styles()->style, 0);
    lv_obj_add_style(g_btn_app_hint, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(g_btn_app_hint, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(g_btn_app_hint, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_align(g_btn_app_hint, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_t *lab_app_inst = lv_label_create(g_btn_app_hint);
    lv_label_set_text_static(lab_app_inst, "To install APP");
    lv_obj_set_style_text_color(lab_app_inst, lv_color_make(18, 18, 18), LV_STATE_DEFAULT);
    lv_obj_align(lab_app_inst, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_btn_app_hint, ui_net_config_page_app_click_cb, LV_EVENT_CLICKED, NULL);
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), g_btn_app_hint);
    }

    g_hint_lab = lv_label_create(g_page);
    lv_label_set_recolor(g_hint_lab, true);
    lv_label_set_text_static(g_hint_lab, "...");
    lv_obj_align(g_hint_lab, LV_ALIGN_CENTER, 0, 0);
    ui_net_config_update_cb(g_net_state, NULL);
}
