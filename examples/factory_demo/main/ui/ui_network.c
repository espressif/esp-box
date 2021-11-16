/**
 * @file ui_network.c
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
#include "app_network.h"
#include "lvgl.h"
#include "ui_main.h"

LV_FONT_DECLARE(font_en_16)
LV_IMG_DECLARE(esp_logo_tiny)

static lv_obj_t *qr = NULL;
static lv_obj_t *lab_net_state = NULL;
static lv_obj_t *label_title = NULL;

static const char *wifi_info = "WIFI:T:WPA;S:" EXAMPLE_ESP_WIFI_SSID ";P:" EXAMPLE_ESP_WIFI_PASS ";";
static const char *web_page = "http://192.168.4.1/";
static const char *dev_dis_msg = "1. AP: \"" EXAMPLE_ESP_WIFI_SSID "\", PWD: \"" EXAMPLE_ESP_WIFI_PASS "\"";
static const char *dev_con_msg = "2. Visit web \"http://192.168.4.1\"\n" "  and wait for around 1 minute";
static const char *ui_title_default = "Scan the QRcode to connect";
static const char *ui_title_connetced = "Scan again to visit web";
static char *qr_data = NULL;
static char *net_msg = NULL;
static bool s_connect = false;

static void btn_back_network_cb(lv_event_t *event)
{
    ui_network(false);
    ui_clock(true);
}

void ui_network(bool show)
{
    static lv_obj_t *mask = NULL;

    if (NULL == mask) {
        mask = lv_obj_create(lv_scr_act());
        lv_obj_set_style_radius(mask, 20, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(mask, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(mask, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_size(mask, 290, 180);
        lv_obj_align(mask, LV_ALIGN_BOTTOM_MID, 0, -15);
        lv_obj_clear_flag(mask, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
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

    if (NULL == label_title) {
        label_title = lv_label_create(mask);
        lv_label_set_text_static(label_title, s_connect ? ui_title_connetced : ui_title_default);
        lv_obj_set_style_text_font(label_title, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_align_to(label_title, btn_back, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }

    if (NULL == qr) {
        if (NULL == qr_data) {
            qr_data = (char *) wifi_info;
        }
        qr = lv_qrcode_create(mask, 92, lv_color_black(), lv_color_white());
        lv_qrcode_update(qr, qr_data, strlen(qr_data));
        lv_obj_align(qr, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *img = lv_img_create(qr);
        lv_img_set_src(img, &esp_logo_tiny);
        lv_obj_center(img);
    }

    lv_qrcode_update(qr, qr_data, strlen(qr_data));

    if (NULL == lab_net_state) {
        if (NULL == net_msg) {
           net_msg = (char *) dev_dis_msg;
        }
        lab_net_state = lv_label_create(mask);
        lv_label_set_text_static(lab_net_state, net_msg);
        lv_obj_set_style_text_font(lab_net_state, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_align(lab_net_state, LV_ALIGN_BOTTOM_MID, 0, 0);
    }

    lv_label_set_text_static(lab_net_state, net_msg);

    if (show) {
        lv_obj_clear_flag(mask, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mask, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_network_set_state(bool connect)
{
    s_connect = connect;

    if (connect) {
        qr_data = (char *) web_page;
        net_msg = (char *) dev_con_msg;
    } else {
        qr_data = (char *) wifi_info;
        net_msg = (char *) dev_dis_msg;
    }

    if (NULL != qr) {
        lv_qrcode_update(qr, qr_data, strlen(qr_data));
    }

    if (NULL != lab_net_state) {
        lv_label_set_text_static(lab_net_state, net_msg);
    }

    if (NULL != label_title) {
        lv_label_set_text_static(label_title, connect ? ui_title_connetced : ui_title_default);
    }
}
