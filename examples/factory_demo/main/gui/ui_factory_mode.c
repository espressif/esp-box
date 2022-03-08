/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_log.h"
#include "bsp_board.h"
#include "lvgl/lvgl.h"
#include "app_sr.h"
#include "ui_main.h"
#include "settings.h"

static const char *TAG = "ui_factory";

static void (*g_factory_end_cb)(void) = NULL;
static uint32_t g_active_index = 0;
typedef struct {
    sr_language_t sr_lang;
    const char *text;
} language_info_t;
static const language_info_t g_lang_info[] = {
    {SR_LANG_EN, "English"},
    {SR_LANG_CN, "Chinese"},
};

static void ui_factory_page_return_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    if (ui_get_btn_op_group()) {
        lv_group_remove_all_objs(ui_get_btn_op_group());
    }
    lv_obj_del(obj);
    if (g_factory_end_cb) {
        g_factory_end_cb();
    }
}

static lv_obj_t *create_wait_page(lv_obj_t *page)
{
    lv_obj_t *cont1 = lv_obj_create(page);
    // lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_size(cont1, lv_pct(80), lv_pct(40));
    lv_obj_t *lab_text = lv_label_create(cont1);
    lv_label_set_text_static(lab_text, "Processing...");
    lv_obj_align(lab_text, LV_ALIGN_CENTER, 0, 0);
    return cont1;
}

static void ui_factory_page_save_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    lv_obj_t *page = create_wait_page(obj);
    lv_task_handler(); vTaskDelay(50);
    sys_param_t *param = settings_get_parameter();
    param->sr_lang = g_lang_info[g_active_index - 1].sr_lang;
    param->need_hint = 1; //
    settings_write_parameter_to_nvs();
    app_sr_set_language(param->sr_lang);
    lv_task_handler(); vTaskDelay(50);
    lv_obj_del(page);
    ui_factory_page_return_click_cb(e);
}

static void radio_event_handler(lv_event_t *e)
{
    uint32_t *active_id = lv_event_get_user_data(e);
    lv_obj_t *cont = lv_event_get_current_target(e);
    lv_obj_t *act_cb = lv_event_get_target(e);
    lv_obj_t *old_cb = lv_obj_get_child(cont, *active_id);

    /*Do nothing if the container was clicked*/
    if (act_cb == cont) {
        return;
    }

    lv_obj_clear_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/

    *active_id = lv_obj_get_index(act_cb);

    ESP_LOGI(TAG, "Selected language: %s", g_lang_info[g_active_index - 1].text);
    sys_param_t *param = settings_get_parameter();
    param->sr_lang = g_lang_info[g_active_index - 1].sr_lang;
}


static lv_obj_t *radiobutton_create(lv_obj_t *parent, const char *txt)
{
    lv_obj_t *obj = lv_checkbox_create(parent);
    lv_checkbox_set_text(obj, txt);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(obj, NULL, LV_PART_INDICATOR | LV_STATE_CHECKED);

    // lv_style_init(&style_radio_chk);
    // lv_style_set_bg_img_src(&style_radio_chk, NULL);
    // lv_obj_add_style(obj, &style_radio, LV_PART_INDICATOR);
    // lv_obj_add_style(obj, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);
    return obj;
}

/**
 * Checkboxes as radio buttons
 */
static lv_obj_t *create_lang_checkboxs(lv_obj_t *page)
{
    /* The idea is to enable `LV_OBJ_FLAG_EVENT_BUBBLE` on checkboxes and process the
     * `LV_EVENT_CLICKED` on the container.
     * A variable is passed as event user data where the index of the active
     * radiobutton is saved */

    uint32_t i;
    char buf[32];

    lv_obj_t *cont1 = lv_obj_create(page);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(cont1, lv_pct(40), lv_pct(80));
    lv_obj_add_event_cb(cont1, radio_event_handler, LV_EVENT_CLICKED, &g_active_index);
    lv_obj_t *lab_text = lv_label_create(cont1);
    lv_label_set_text_static(lab_text, "Language");
    lv_obj_align(lab_text, LV_ALIGN_TOP_MID, 0, 0);

    for (i = 0; i < sizeof(g_lang_info) / sizeof(language_info_t); i++) {
        lv_snprintf(buf, sizeof(buf), "%s", g_lang_info[i].text);
        lv_obj_t *obj = radiobutton_create(cont1, buf);
        if (ui_get_btn_op_group()) {
            lv_group_add_obj(ui_get_btn_op_group(), obj);
        }
    }
    /*Make the first checkbox checked*/
    sys_param_t *param = settings_get_parameter();
    g_active_index = param->sr_lang + 1;
    lv_obj_add_state(lv_obj_get_child(cont1, g_active_index), LV_STATE_CHECKED);
    return cont1;
}

void ui_factory_start(void (*fn)(void))
{
    g_factory_end_cb = fn;

    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, lv_obj_get_width(lv_obj_get_parent(page)), lv_obj_get_height(lv_obj_get_parent(page)));
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(page, lv_color_make(200, 255, 200), LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(page, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *lab_text = lv_label_create(page);
    lv_label_set_text_static(lab_text, "Factory Mode");
    lv_obj_align(lab_text, LV_ALIGN_TOP_MID, 0, 0);

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
    lv_obj_add_event_cb(btn_return, ui_factory_page_return_click_cb, LV_EVENT_CLICKED, page);
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_return);
    }

    lv_obj_t *cont = create_lang_checkboxs(page);
    lv_obj_align(cont, LV_ALIGN_TOP_LEFT, 0, 30);

    lv_obj_t *btn_save = lv_btn_create(page);
    lv_obj_set_size(btn_save, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(btn_save, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_save, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_save, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_save, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lab_btn_text = lv_label_create(btn_save);
    lv_label_set_text_static(lab_btn_text, "Confirm");
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(255, 0, 0), LV_STATE_DEFAULT);
    lv_obj_center(lab_btn_text);
    lv_obj_add_event_cb(btn_save, ui_factory_page_save_click_cb, LV_EVENT_CLICKED, page);
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_save);
    }
    lv_group_focus_obj(btn_save);
}
