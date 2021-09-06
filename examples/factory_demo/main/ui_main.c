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
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#ifndef PI
#define PI  (3.14159f)
#endif

static const char *TAG = "ui_main";

static void boot_animate_start(lv_obj_t *scr);

esp_err_t ui_main_start(void)
{
    if (NULL == lv_scr_act()) {
        ESP_LOGE(TAG, "LVGL not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    boot_animate_start(lv_scr_act());

    return ESP_OK;
}

/* **************** SR DEMO UI **************** */
static lv_obj_t *img_alexa = NULL;
static lv_obj_t *label_cmd = NULL;

#if CONFIG_SR_ENGLISH
static const char *cmd_list[] = {
    "Tell me a joke",
    "Sing a song",
    "Play news channel",
    "Turn on my soundbox",
    "Turn off my soundbox",
    "Highest volume",
    "Lowest volume",
    "Increase the volume",
    "Decrease the volume",
    "Turn on the TV",
    "Turn off the TV",
    "Make me a tea",
    "Make me a coffee",
    "Turn on the light",
    "Turn off the light",
    "Change the color\n"
    "to red",
    "Change the color\n"
    "to green",
    "Turn on\n"
    "all the lights",
    "Turn off\n"
    "all the lights",
    "Turn on the\n"
    "air conditioner",
    "Turn off the\n"
    "air conditioner",
    "Set the temprature\n"
    "to 16 degrees",
    "Set the temprature\n"
    "to 17 degrees",
    "Set the temprature\n"
    "to 18 degrees",
    "Set the temprature\n"
    "to 19 degrees",
    "Set the temprature\n"
    "to 20 degrees",
    "Set the temprature\n"
    "to 21 degrees",
    "Set the temprature\n"
    "to 22 degrees",
    "Set the temprature\n"
    "to 23 degrees",
    "Set the temprature\n"
    "to 24 degrees",
    "Set the temprature\n"
    "to 25 degrees",
    "Set the temprature\n"
    "to 26 degrees",
    "Invalid command",
};
#elif CONFIG_SR_CHINESE
static const char *cmd_list[] = {
    "打开空调",
    "关闭空调",
    "增大风速",
    "减小风速",
    "升高一度",
    "降低一度",
    "制热模式",
    "制冷模式",
    "送风模式",
    "节能模式",
    "除湿模式",
    "健康模式",
    "睡眠模式",
    "打开蓝牙",
    "关闭蓝牙",
    "开始播放",
    "暂停播放",
    "定时一小时",
    "打开电灯",
    "关闭电灯",
    "错误命令",
};
#else
    #error "Please select language of command word"
#endif

LV_IMG_DECLARE(alexa)
LV_FONT_DECLARE(font_cmd_28)

static void ui_sr_start(void)
{
    label_cmd = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_cmd, &font_cmd_28, 0);
    lv_label_set_text(label_cmd, "Say Alexa");
    lv_obj_align(label_cmd, LV_ALIGN_CENTER, 0, -40);

    img_alexa = lv_img_create(lv_scr_act());
    lv_obj_add_flag(img_alexa, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(img_alexa, &alexa);
    lv_obj_align(img_alexa, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void ui_sr_show_cmd(int cmd_id)
{
    const char *cmd_str = NULL;
    size_t cmd_num = sizeof(cmd_list) / sizeof(cmd_list[0]) - 1;

    /* Check command ID */
    if ((cmd_id < 0) || (cmd_id >= cmd_num)) {
        cmd_str = cmd_list[cmd_num];
    } else {
        cmd_str = cmd_list[cmd_id];
    }

    /* Return if label not created yet */
    if (NULL == label_cmd) {
        return;
    }

    /* Update command */
    lv_label_set_text(label_cmd, cmd_str);
}

void ui_sr_show_icon(bool show)
{
    if (NULL ==img_alexa) {
        return;
    }

    if (show) {
        lv_obj_clear_flag(img_alexa, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(img_alexa, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_sr_show_text(char *text)
{
     /* Return if label not created yet */
    if (NULL == label_cmd) {
        return;
    }

    lv_label_set_text(label_cmd, text);
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
        ui_sr_start();
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
