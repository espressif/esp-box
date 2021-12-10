/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_log.h"
#include "bsp_board.h"
#include "lvgl/lvgl.h"
#include "bsp_btn.h"
#include "ui_main.h"
#include "settings.h"

static const char *TAG = "ui_hint";

#define HINT_PAGE_NUM 4
static lv_obj_t *g_hint_page_btn[HINT_PAGE_NUM] = {0};
static void (*g_hint_end_cb)(void) = NULL;

LV_FONT_DECLARE(font_en_16);
LV_FONT_DECLARE(font_cn_gb1_16);


static void hint_page_btn_prev_cb(void *arg)
{
    button_dev_t *btn = (button_dev_t *) arg;
    lv_obj_t *obj = (lv_obj_t *) btn->cb_user_data;
    uint16_t tab_index = lv_tabview_get_tab_act(obj);

    if (tab_index > 0) {
        tab_index --;
        ESP_LOGI(TAG, "hint previous");
        lv_tabview_set_act(obj, tab_index, LV_ANIM_ON);
        if (ui_get_btn_op_group()) {
            lv_group_remove_all_objs(ui_get_btn_op_group());
            lv_group_add_obj(ui_get_btn_op_group(), g_hint_page_btn[tab_index]);
        }
    }
}
static void hint_page_btn_next_cb(void *arg)
{
    button_dev_t *btn = (button_dev_t *) arg;
    lv_obj_t *obj = (lv_obj_t *) btn->cb_user_data;
    uint16_t tab_index = lv_tabview_get_tab_act(obj);

    if (tab_index < HINT_PAGE_NUM - 1) {
        tab_index ++;
        ESP_LOGI(TAG, "hint next");
        lv_tabview_set_act(obj, tab_index, LV_ANIM_ON);
        if (ui_get_btn_op_group()) {
            lv_group_remove_all_objs(ui_get_btn_op_group());
            lv_group_add_obj(ui_get_btn_op_group(), g_hint_page_btn[tab_index]);
        }
    }
}
static void ui_hint_page_next_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    uint16_t tab_index = lv_tabview_get_tab_act(obj);
    if (HINT_PAGE_NUM - 1 == tab_index && g_hint_end_cb) {
        if (ui_get_btn_op_group()) {
            lv_group_remove_all_objs(ui_get_btn_op_group());
        }
        ui_btn_rm_all_cb();
        lv_obj_del(obj);
        g_hint_end_cb();
    } else {
        button_dev_t btn = {
            .cb_user_data = obj,
        };
        hint_page_btn_next_cb(&btn);
    }
}


void ui_hint_start(void (*fn)(void))
{
    g_hint_end_cb = fn;

    const board_res_desc_t *brd = bsp_board_get_description();
    /*Create a Tab view object*/
    lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 0);
    LV_IMG_DECLARE(hand_down);
    LV_IMG_DECLARE(hand_left);
    lv_obj_t *hint_page = NULL;

    /*Add content to the tabs*/

    // ======= Content of Tab1
    hint_page = lv_tabview_add_tab(tabview, "Tab 1");
    lv_obj_set_scrollbar_mode(hint_page, LV_SCROLLBAR_MODE_OFF);

    static const char *hint_msg2[] = {
        "Go\n"
        "Backward",
        "Click to\n"
        "confirm",
        "Go\n"
        "Forward",
    };

    if (!brd->BSP_INDEV_IS_TP) {
        // For S3 BOX LITE
        for (int i = 0; i < 3; i++) {
            lv_obj_t *lab_hint = lv_label_create(hint_page);
            lv_label_set_text_static(lab_hint, hint_msg2[i]);
            lv_obj_set_style_text_align(lab_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            lv_obj_align(lab_hint, LV_ALIGN_CENTER, 100 * i - 100, -10);
            lv_obj_t *img = lv_img_create(hint_page);
            lv_img_set_src(img, &hand_down);
            lv_obj_align_to(img, lab_hint, LV_ALIGN_BOTTOM_MID, 0, 60);
        }

        static const lv_point_t line_point[] = { {.x = 0, .y = 0}, {.x = 0, .y = 170} };
        for (int i = 0; i < 2; i++) {
            lv_obj_t *line = lv_line_create(hint_page);
            lv_line_set_points(line, line_point, 2);
            lv_obj_set_style_line_width(line, 1, LV_PART_MAIN);
            lv_obj_set_style_line_color(line, lv_color_make(222, 230, 243), LV_STATE_DEFAULT);
            lv_obj_align(line, LV_ALIGN_CENTER, 100 * i - 50, 0);
        }
    } else {
        // For S3 BOX
        lv_obj_t *lab_hint = lv_label_create(hint_page);
        lv_label_set_text_static(lab_hint, "Touch to return");
        lv_obj_set_style_text_align(lab_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(lab_hint, LV_ALIGN_CENTER, 0, -10);
        lv_obj_t *img = lv_img_create(hint_page);
        lv_img_set_src(img, &hand_down);
        lv_obj_align_to(img, lab_hint, LV_ALIGN_BOTTOM_MID, 0, 60);
    }

    lv_obj_t *lab_index = lv_label_create(hint_page);
    lv_label_set_text_fmt(lab_index, "%d/%d", 1, HINT_PAGE_NUM);
    lv_obj_align(lab_index, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *btn_next = g_hint_page_btn[0] = lv_btn_create(hint_page);
    lv_obj_add_style(btn_next, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(btn_next, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -10, 0);
    lv_obj_t *label = lv_label_create(btn_next);
    lv_label_set_text(label, "Next");
    lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_next, ui_hint_page_next_click_cb, LV_EVENT_RELEASED, tabview);

    if (ui_get_btn_op_group()) {
        // Add the button of first page
        lv_group_remove_all_objs(ui_get_btn_op_group());
        lv_group_add_obj(ui_get_btn_op_group(), g_hint_page_btn[0]);
    }


    // ======= Content of Tab2
    hint_page = lv_tabview_add_tab(tabview, "Tab 2");
    lv_obj_set_scrollbar_mode(hint_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *img = lv_img_create(hint_page);
    lv_img_set_src(img, &hand_left);
    lv_obj_align(img, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *lab_btn_name = lv_label_create(hint_page);
    lv_label_set_recolor(lab_btn_name, true);
    lv_label_set_text_static(lab_btn_name, "#000000 Function Button#");
    lv_obj_align_to(lab_btn_name, img, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_t *lab_hint = lv_label_create(hint_page);
    lv_label_set_text_static(lab_hint, "Customized by user");
    lv_obj_align_to(lab_hint, lab_btn_name, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    img = lv_img_create(hint_page);
    lv_img_set_src(img, &hand_left);
    lv_obj_align(img, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lab_btn_name = lv_label_create(hint_page);
    lv_label_set_recolor(lab_btn_name, true);
    lv_label_set_text_static(lab_btn_name, "#000000 Reset Button#");
    lv_obj_align_to(lab_btn_name, img, LV_ALIGN_OUT_RIGHT_MID, 10, -40);
    lab_hint = lv_label_create(hint_page);
    lv_label_set_text_static(lab_hint, "Press to reset\nthe device");
    lv_obj_align_to(lab_hint, lab_btn_name, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    lab_index = lv_label_create(hint_page);
    lv_label_set_text_fmt(lab_index, "%d/%d", 2, HINT_PAGE_NUM);
    lv_obj_align(lab_index, LV_ALIGN_BOTTOM_MID, 0, 0);

    btn_next = g_hint_page_btn[1] = lv_btn_create(hint_page);
    lv_obj_add_style(btn_next, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(btn_next, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -10, 0);
    label = lv_label_create(btn_next);
    lv_label_set_text(label, "Next");
    lv_obj_set_style_text_color(label, lv_color_make(255, 0, 0), LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_next, ui_hint_page_next_click_cb, LV_EVENT_RELEASED, tabview);

    // ======= Content of Tab3
    hint_page = lv_tabview_add_tab(tabview, "Tab 3");
    lv_obj_set_scrollbar_mode(hint_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(hint_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(hint_page, lv_color_white(), LV_STATE_DEFAULT);
    // lv_obj_set_style_radius(hint_page, 15, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(hint_page, 0, LV_STATE_DEFAULT);
    // lv_obj_set_style_shadow_width(hint_page, 100, LV_STATE_DEFAULT);
    // lv_obj_set_style_shadow_opa(hint_page, LV_OPA_30, LV_STATE_DEFAULT);
    lv_obj_center(hint_page);

    lv_obj_t *label_title = lv_label_create(hint_page);
    lv_obj_set_style_text_color(label_title, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_title, &font_en_16, LV_STATE_DEFAULT);
    lv_label_set_recolor(label_title, true);
    lv_label_set_text_static(label_title, "#000000 Steps for Voice Assistant#");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *label_hint = lv_label_create(hint_page);
    lv_obj_set_style_text_line_space(label_hint, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hint, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    const sys_param_t *param = settings_get_parameter();
    if (SR_LANG_EN == param->sr_lang) {
        lv_obj_set_style_text_font(label_hint, &font_en_16, LV_STATE_DEFAULT);
        static const char msg[] = "1: Say \"Hi E. S. P.\" to wake-up the device.\n"
                                  "2: Wait for the \"Hi ESP\" shows on screen.\n"
                                  "3: Say command, like \"turn on the light\".";
        lv_label_set_text_static(label_hint, msg);
    } else {
        lv_obj_set_style_text_font(label_hint, &font_cn_gb1_16, LV_STATE_DEFAULT);
        static const char msg[] = "1: Say \"Hi 乐鑫\" to wake-up the device.\n"
                                  "2: Wait for the \"Hi 乐鑫\" shows on screen.\n"
                                  "3: Say command, like \"打开电灯\".";
        lv_label_set_text_static(label_hint, msg);
    }
    lv_obj_align_to(label_hint, label_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lab_index = lv_label_create(hint_page);
    lv_label_set_text_fmt(lab_index, "%d/%d", 3, HINT_PAGE_NUM);
    lv_obj_align(lab_index, LV_ALIGN_BOTTOM_MID, 0, 0);

    btn_next = g_hint_page_btn[2] = lv_btn_create(hint_page);
    lv_obj_add_style(btn_next, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(btn_next, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(btn_next, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -10, 0);
    label = lv_label_create(btn_next);
    lv_label_set_text(label, "Next");
    lv_obj_set_style_text_color(label, lv_color_make(255, 0, 0), LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_next, ui_hint_page_next_click_cb, LV_EVENT_RELEASED, tabview);

    // ======= Content of Tab4
    hint_page = lv_tabview_add_tab(tabview, "Tab 4");
    lv_obj_set_scrollbar_mode(hint_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(hint_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(hint_page, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(hint_page, 0, LV_STATE_DEFAULT);
    lv_obj_center(hint_page);

    label_title = lv_label_create(hint_page);
    lv_obj_set_style_text_color(label_title, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_title, &font_en_16, LV_STATE_DEFAULT);
    lv_label_set_recolor(label_title, true);
    lv_label_set_text_static(label_title, "#000000 Default Command Words:#");
    lv_obj_align(label_title, LV_ALIGN_TOP_LEFT, 0, 30);

    lv_obj_t *label_cmd_hint = lv_label_create(hint_page);
    lv_label_set_recolor(label_cmd_hint, true);
    if (SR_LANG_EN == param->sr_lang) {
        lv_obj_set_style_text_font(label_cmd_hint, &font_en_16, LV_STATE_DEFAULT);
        static const char cmd_msg[] = "\"Turn on the light\", \"Switch off the light\"\n"
                                      "\"Turn Red\", \"Turn Green\", \"Turn Blue\"\n"
                                      "\"Sing a song\", \"Next Song\"\n"
                                      "\"Pause Playing\"";
        lv_label_set_text_static(label_cmd_hint, cmd_msg);
    } else {
        lv_obj_set_style_text_font(label_cmd_hint, &font_cn_gb1_16, LV_STATE_DEFAULT);
        static const char cmd_msg[] = "\"打开电灯\", 关闭电灯\"\n"
                                      "\"调成红色\", 调成绿色\", 调成蓝色\"\n"
                                      "\"播放音乐\", 切歌\", 暂停播放";
        lv_label_set_text_static(label_cmd_hint, cmd_msg);
    }
    lv_obj_set_style_text_line_space(label_cmd_hint, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label_cmd_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_cmd_hint, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_align_to(label_cmd_hint, label_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

    lab_index = lv_label_create(hint_page);
    lv_label_set_text_fmt(lab_index, "%d/%d", 4, HINT_PAGE_NUM);
    lv_obj_align(lab_index, LV_ALIGN_BOTTOM_MID, 0, 0);

    btn_next = g_hint_page_btn[3] = lv_btn_create(hint_page);
    lv_obj_add_style(btn_next, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(btn_next, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(btn_next, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(btn_next, LV_ALIGN_CENTER, 0, 60);
    label = lv_label_create(btn_next);
    lv_label_set_text(label, "OK Let's Go");
    lv_obj_set_style_text_color(label, lv_color_make(255, 0, 0), LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_next, ui_hint_page_next_click_cb, LV_EVENT_RELEASED, tabview);

    if (!brd->BSP_INDEV_IS_TP) {
        bsp_btn_register_callback(BOARD_BTN_ID_PREV, BUTTON_PRESS_DOWN, hint_page_btn_prev_cb, tabview);
        bsp_btn_register_callback(BOARD_BTN_ID_NEXT, BUTTON_PRESS_DOWN, hint_page_btn_next_cb, tabview);
    }

}


