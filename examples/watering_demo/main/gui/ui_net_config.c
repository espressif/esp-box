/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"
#include "bsp_board.h"
#include "lvgl.h"
#include "app_wifi.h"
#include "ui_main.h"

static const char *TAG = "ui_net_config";

static lv_obj_t *g_btn_app_hint = NULL;

static void (*g_net_config_end_cb)(void) = NULL;

static void ui_app_page_return_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    if (ui_get_btn_op_group()) {
        lv_group_focus_freeze(ui_get_btn_op_group(), false);
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
    lv_obj_del(obj);
    if (g_net_config_end_cb) {
        g_net_config_end_cb();
    }
}

void ui_net_config_start(void (*fn)(void))
{
    g_net_config_end_cb = fn;

    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, 290, 190);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(page, 15, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(page, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(page, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(page, LV_OPA_30, LV_PART_MAIN);
    lv_obj_align(page, LV_ALIGN_TOP_MID, 0, 40);

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
    lv_obj_add_event_cb(btn_return, ui_net_config_page_return_click_cb, LV_EVENT_CLICKED, page);
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_return);
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
    lv_obj_add_event_cb(g_btn_app_hint, ui_net_config_page_app_click_cb, LV_EVENT_CLICKED, page);
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), g_btn_app_hint);
    }

    /* **************** QR CODE **************** */
    LV_IMG_DECLARE(esp_logo_tiny);
    const char *prov_msg = app_wifi_get_prov_payload();
    size_t prov_msg_len = strlen(prov_msg);
    lv_obj_t *qr = lv_qrcode_create(page, 92, lv_color_black(), lv_color_white());
    ESP_LOGI(TAG, "QR Data: %s", prov_msg);
    lv_qrcode_update(qr, prov_msg, prov_msg_len);
    lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t *img = lv_img_create(qr);
    lv_img_set_src(img, &esp_logo_tiny);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    /* **************** PROV HINT **************** */
    lv_obj_t *prov_hint = lv_label_create(page);
    lv_label_set_text_static(prov_hint,
                             "1.Open ESP-BOX APP on your phone\n"
                             "2.Scan the QR Code to provision");
    lv_obj_align_to(prov_hint, qr, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_t *lab_prov_state = lv_label_create(page);
    if (app_wifi_is_connected()) {
        ui_main_status_bar_set_wifi(1);
        lv_label_set_text_static(lab_prov_state,
                                 "Device alreay configured!");
    } else {
        ui_main_status_bar_set_wifi(0);
        lv_label_set_text_static(lab_prov_state,
                                 "Wait for Provision...");
    }
    lv_obj_align(lab_prov_state, LV_ALIGN_BOTTOM_MID, 0, 0);

}
